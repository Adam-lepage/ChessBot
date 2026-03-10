#pragma once

#include "ChessBot.h"
#include "Evaluation.h"
#include "Game.h"
#include <vector>
#include <string>
#include <iostream>
#include <chrono>
#include <climits>
#include <algorithm>
#include <random>

class Botv3 : public ChessBot {
public:
    static constexpr int MAX_DEPTH = 5;
    static constexpr int MAX_QDEPTH = 4;

    Botv3() : rng(std::random_device{}()) {}

    void setMaxDepth(int depth) override { maxDepth = depth; }
    int getMaxDepth() const { return maxDepth; }

    Move chooseMove(const BitboardEngine&, MoveValidator& validator, int color) override {
        BitboardEngine* eng = validator.getEngine();

        std::vector<Move> rootMoves = generateAllMoves(*eng, validator, color);
        if (rootMoves.empty()) return Move(0, 0, 0, 0);

        Move bestMove = rootMoves[0];

        struct DepthStats { int positions; long long timeMs; int eval; };
        std::vector<DepthStats> stats;

        for (int depth = 1; depth <= maxDepth; depth++) {
            auto start = std::chrono::high_resolution_clock::now();
            positionsEvaluated = 0;

            orderMoves(rootMoves, *eng, bestMove);

            int alpha = -100000000;
            int beta  =  100000000;
            int bestEval = -100000000;
            Move depthBest = rootMoves[0];

            for (auto& rootMove : rootMoves) {
                EngineState engState = saveEngineState(*eng);
                MoveValidator::ValidatorState valState = validator.getState();

                Move m = rootMove;
                if (!validator.executeMove(m, color, true)) {
                    restoreEngineState(*eng, engState);
                    validator.setState(valState);
                    continue;
                }

                int eval = -negamax(validator, *eng, depth - 1, 1 - color, -beta, -alpha);

                restoreEngineState(*eng, engState);
                validator.setState(valState);

                if (eval > bestEval) {
                    bestEval = eval;
                    depthBest = rootMove;
                }

                if (eval > alpha) alpha = eval;
            }

            bestMove = depthBest;

            auto end = std::chrono::high_resolution_clock::now();
            long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
            stats.push_back({positionsEvaluated, ms, bestEval});
        }

        if (g_debugOutput) {
            std::cout << "\n=== Botv3 Search ===" << std::endl;
            for (int i = 0; i < (int)stats.size(); i++) {
                std::cout << "  Depth " << (i + 1)
                          << ": " << stats[i].positions << " positions"
                          << ", " << stats[i].timeMs << "ms"
                          << ", eval=" << stats[i].eval << std::endl;
            }
            std::cout << "  Best: "
                      << BitboardEngine::squareToAlgebraic(bestMove.fromRow, bestMove.fromCol)
                      << " -> "
                      << BitboardEngine::squareToAlgebraic(bestMove.toRow, bestMove.toCol)
                      << "\n" << std::endl;
        }

        return bestMove;
    }

    std::string getName() const override { return "Botv3"; }

private:
    mutable std::mt19937 rng;
    int maxDepth = MAX_DEPTH;
    int positionsEvaluated = 0;

    // Large finite value used as -infinity sentinel.
    // Must NOT be INT_MIN or -INT_MAX: negating those causes overflow/UB
    // when the parent does -negamax(...) or passes -alpha/-beta.
    static constexpr int NEG_INF = -100000000;
    static constexpr int POS_INF =  100000000;

    int negamax(MoveValidator& validator, BitboardEngine& eng, int depth, int currentColor, int alpha, int beta) {
        if (depth == 0) {
            return quiescence(validator, eng, currentColor, alpha, beta, 0);
        }

        std::vector<Move> moves = generateAllMoves(eng, validator, currentColor);

        if (moves.empty()) {
            if (validator.isKingInCheck(currentColor)) {
                return -100000 - depth;  // Checkmate: more negative = mated sooner = worse
            }
            return 0;  // Stalemate
        }

        Move noMove(0, 0, 0, 0);
        orderMoves(moves, eng, noMove);

        int best = NEG_INF;
        bool anyMoveMade = false;

        for (auto& move : moves) {
            EngineState engState = saveEngineState(eng);
            MoveValidator::ValidatorState valState = validator.getState();
            Move m = move;

            // FIX 1: if executeMove rejects a generated move, skip it cleanly
            // rather than evaluating the unchanged (wrong) position.
            // Without this, best stays at NEG_INF and the parent sees an
            // enormous score after negation, corrupting the entire search.
            if (!validator.executeMove(m, currentColor, true)) {
                restoreEngineState(eng, engState);
                validator.setState(valState);
                continue;
            }

            anyMoveMade = true;
            int eval = -negamax(validator, eng, depth - 1, 1 - currentColor, -beta, -alpha);
            restoreEngineState(eng, engState);
            validator.setState(valState);

            if (eval > best)  best = eval;
            if (eval > alpha) alpha = eval;
            if (alpha >= beta) break;  // Beta cutoff
        }

        // FIX 1 (continued): if every generated move was rejected by executeMove,
        // fall back to a correct terminal score rather than returning NEG_INF.
        if (!anyMoveMade) {
            if (validator.isKingInCheck(currentColor)) {
                return -100000 - depth;
            }
            return 0;
        }

        return best;
    }

    int quiescence(MoveValidator& validator, BitboardEngine& eng, int currentColor, int alpha, int beta, int qDepth) {
        positionsEvaluated++;

        bool inCheck = validator.isKingInCheck(currentColor);

        // Hard depth cap — return static eval with penalty if still in check
        if (qDepth >= MAX_QDEPTH) {
            int score = evaluate(eng);
            score = (currentColor == 0) ? score : -score;
            if (inCheck) score -= 500; // penalize unresolved check
            return score;
        }

        if (inCheck) {
            // In check: must find an escape. Stand-pat is unsound here because
            // the side to move MUST make a move — the current position isn't an
            // option. Search ALL legal moves (not just captures) to find evasions.
            int best = NEG_INF;

            std::vector<Move> moves = generateAllMoves(eng, validator, currentColor);
            if (moves.empty()) {
                // Checkmate
                return -100000 + qDepth;
            }

            Move noMove(0, 0, 0, 0);
            orderMoves(moves, eng, noMove);

            bool anyMoveMade = false;
            for (auto& move : moves) {
                EngineState engState = saveEngineState(eng);
                MoveValidator::ValidatorState valState = validator.getState();
                Move m = move;

                if (!validator.executeMove(m, currentColor, true)) {
                    restoreEngineState(eng, engState);
                    validator.setState(valState);
                    continue;
                }

                anyMoveMade = true;
                int eval = -quiescence(validator, eng, 1 - currentColor, -beta, -alpha, qDepth + 1);

                restoreEngineState(eng, engState);
                validator.setState(valState);

                if (eval > best)  best = eval;
                if (best >= beta) return best;
                if (best > alpha) alpha = best;
            }

            if (!anyMoveMade) {
                return -100000 + qDepth; // checkmate
            }

            return best;
        }

        // Not in check: normal quiescence with stand-pat + captures only
        int standPat = evaluate(eng);
        standPat = (currentColor == 0) ? standPat : -standPat;

        int best = standPat;
        if (best >= beta)  return best;
        if (best > alpha)  alpha = best;

        std::vector<Move> moves = generateCaptureMoves(eng, validator, currentColor);
        if (moves.empty()) return best;

        Move noMove(0, 0, 0, 0);
        orderMoves(moves, eng, noMove);

        for (auto& move : moves) {
            EngineState engState = saveEngineState(eng);
            MoveValidator::ValidatorState valState = validator.getState();
            Move m = move;

            if (!validator.executeMove(m, currentColor, true)) {
                restoreEngineState(eng, engState);
                validator.setState(valState);
                continue;
            }

            int eval = -quiescence(validator, eng, 1 - currentColor, -beta, -alpha, qDepth + 1);

            restoreEngineState(eng, engState);
            validator.setState(valState);

            if (eval > best)  best = eval;
            if (best >= beta) return best;
            if (best > alpha) alpha = best;
        }

        return best;
    }

    static int scoreMove(const Move& move, const BitboardEngine& eng, const Move& prevBest) {
        if (move.fromRow == prevBest.fromRow && move.fromCol == prevBest.fromCol &&
            move.toRow   == prevBest.toRow   && move.toCol   == prevBest.toCol) {
            return 100000;
        }

        if (move.promotedTo != -1) {
            return 50000 + pieceValue(move.promotedTo);
        }

        int score = 0;

        int captured = eng.getPieceAt(move.toRow, move.toCol);
        if (captured != -1) {
            int attacker = eng.getPieceAt(move.fromRow, move.fromCol);
            score += pieceValue(captured) * 10 - pieceValue(attacker);
            score += 10000;
        }

        return score;
    }

    static int pieceValue(int piece) {
        switch (piece / 2) {
            case 0: return 100;
            case 1: return 500;
            case 2: return 337;
            case 3: return 365;
            case 4: return 1025;
            case 5: return 10000;
            default: return 0;
        }
    }

    static void orderMoves(std::vector<Move>& moves, const BitboardEngine& eng, const Move& prevBest) {
        std::sort(moves.begin(), moves.end(), [&](const Move& a, const Move& b) {
            return scoreMove(a, eng, prevBest) > scoreMove(b, eng, prevBest);
        });
    }

    static int evaluate(const BitboardEngine& eng) {
        return Eval::evaluate(eng);
    }

    std::vector<Move> generateAllMoves(const BitboardEngine& eng, MoveValidator& validator, int color) {
        std::vector<Move> allMoves;
        int promoRank = (color == 0) ? 0 : 7;

        for (int row = 0; row < 8; row++) {
            for (int col = 0; col < 8; col++) {
                int piece = eng.getPieceAt(row, col);
                if (piece == -1) continue;
                int pieceColor = (piece % 2 == 0) ? 0 : 1;
                if (pieceColor != color) continue;

                bool isPawn = (piece / 2 == 0);
                auto moves = validator.getValidMoves(row, col, color);

                for (auto& m : moves) {
                    if (isPawn && m.toRow == promoRank) {
                        int pieces[4] = {
                            (color == 0) ? BitboardEngine::WHITE_QUEEN  : BitboardEngine::BLACK_QUEEN,
                            (color == 0) ? BitboardEngine::WHITE_ROOK   : BitboardEngine::BLACK_ROOK,
                            (color == 0) ? BitboardEngine::WHITE_BISHOP : BitboardEngine::BLACK_BISHOP,
                            (color == 0) ? BitboardEngine::WHITE_KNIGHT : BitboardEngine::BLACK_KNIGHT
                        };
                        for (int p : pieces) {
                            Move pm = m;
                            pm.promotedTo = p;
                            allMoves.push_back(pm);
                        }
                    } else {
                        allMoves.push_back(m);
                    }
                }
            }
        }
        return allMoves;
    }

    std::vector<Move> generateCaptureMoves(const BitboardEngine& eng, MoveValidator& validator, int color) {
        std::vector<Move> captureMoves;
        int promoRank = (color == 0) ? 0 : 7;

        for (int row = 0; row < 8; row++) {
            for (int col = 0; col < 8; col++) {
                int piece = eng.getPieceAt(row, col);
                if (piece == -1) continue;
                int pieceColor = (piece % 2 == 0) ? 0 : 1;
                if (pieceColor != color) continue;

                bool isPawn = (piece / 2 == 0);
                auto moves = validator.getValidMoves(row, col, color);

                for (auto& m : moves) {
                    bool isCapture    = (eng.getPieceAt(m.toRow, m.toCol) != -1);
                    bool isEnPassant  = isPawn && (m.toCol != m.fromCol) && !isCapture;
                    bool isPromotion  = isPawn && m.toRow == promoRank;

                    if (isCapture || isEnPassant || isPromotion) {
                        if (isPromotion) {
                            Move pm = m;
                            pm.promotedTo = (color == 0) ? BitboardEngine::WHITE_QUEEN
                                                         : BitboardEngine::BLACK_QUEEN;
                            captureMoves.push_back(pm);
                        } else {
                            captureMoves.push_back(m);
                        }
                    }
                }
            }
        }
        return captureMoves;
    }

    struct EngineState {
        Bitboard pawns[2], rooks[2], knights[2], bishops[2], queens[2], kings[2];
        Bitboard allWhitePieces, allBlackPieces, allPieces;
    };

    static EngineState saveEngineState(const BitboardEngine& eng) {
        EngineState s;
        for (int i = 0; i < 2; i++) {
            s.pawns[i]   = eng.pawns[i];
            s.rooks[i]   = eng.rooks[i];
            s.knights[i] = eng.knights[i];
            s.bishops[i] = eng.bishops[i];
            s.queens[i]  = eng.queens[i];
            s.kings[i]   = eng.kings[i];
        }
        s.allWhitePieces = eng.allWhitePieces;
        s.allBlackPieces = eng.allBlackPieces;
        s.allPieces      = eng.allPieces;
        return s;
    }

    static void restoreEngineState(BitboardEngine& eng, const EngineState& s) {
        for (int i = 0; i < 2; i++) {
            eng.pawns[i]   = s.pawns[i];
            eng.rooks[i]   = s.rooks[i];
            eng.knights[i] = s.knights[i];
            eng.bishops[i] = s.bishops[i];
            eng.queens[i]  = s.queens[i];
            eng.kings[i]   = s.kings[i];
        }
        eng.allWhitePieces = s.allWhitePieces;
        eng.allBlackPieces = s.allBlackPieces;
        eng.allPieces      = s.allPieces;
    }
};