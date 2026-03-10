#pragma once

#include "ChessBot.h"
#include "Game.h"
#include <vector>
#include <string>
#include <iostream>
#include <chrono>
#include <climits>
#include <algorithm>
#include <random>

class Botv1 : public ChessBot {
public:
    static constexpr int MAX_DEPTH = 6;

    Botv1() : rng(std::random_device{}()) {}

    void setMaxDepth(int depth) override { maxDepth = depth; }
    int getMaxDepth() const { return maxDepth; }

    Move chooseMove(const BitboardEngine&, MoveValidator& validator, int color) override {
        BitboardEngine* eng = validator.getEngine();

        // Generate all legal moves at the root
        std::vector<Move> rootMoves = generateAllMoves(*eng, validator, color);
        if (rootMoves.empty()) return Move(0, 0, 0, 0);

        Move bestMove = rootMoves[0];

        // Stats per depth level
        struct DepthStats { int positions; long long timeMs; int eval; };
        std::vector<DepthStats> stats;

        // Iterative deepening with alpha-beta pruning
        for (int depth = 1; depth <= maxDepth; depth++) {
            auto start = std::chrono::high_resolution_clock::now(); // Start timer
            positionsEvaluated = 0;

            // Order root moves: previous best first, then by MVV-LVA score
            orderMoves(rootMoves, *eng, bestMove);

            int alpha = INT_MIN;
            int beta  = INT_MAX;
            int bestEval = (color == 0) ? INT_MIN : INT_MAX;
            Move depthBest = rootMoves[0];
            std::vector<Move> tiedMoves;  // Moves sharing the best eval

            // For each root move, execute it, then call alphaBeta for opponent's reply
            for (auto& rootMove : rootMoves) {
                // Save full state before making the move
                EngineState engState = saveEngineState(*eng);
                MoveValidator::ValidatorState valState = validator.getState();

                // Execute move
                Move m = rootMove;
                validator.executeMove(m, color, true);

                // Recurse into opponent's reply with alpha-beta window
                int eval = alphaBeta(validator, *eng, depth - 1, 1 - color, alpha, beta);

                // Restore state
                restoreEngineState(*eng, engState);
                validator.setState(valState);

                // White maximizes, black minimizes
                if (color == 0) {
                    if (eval > bestEval) {
                        bestEval = eval; depthBest = rootMove;
                        tiedMoves.clear(); tiedMoves.push_back(rootMove);
                    } else if (eval == bestEval) {
                        tiedMoves.push_back(rootMove);
                    }
                    if (eval > alpha) alpha = eval;
                } else {
                    if (eval < bestEval) {
                        bestEval = eval; depthBest = rootMove;
                        tiedMoves.clear(); tiedMoves.push_back(rootMove);
                    } else if (eval == bestEval) {
                        tiedMoves.push_back(rootMove);
                    }
                    if (eval < beta) beta = eval;
                }
            }

            // Randomly pick among tied best moves for variety
            if (tiedMoves.size() > 1) {
                std::uniform_int_distribution<size_t> dist(0, tiedMoves.size() - 1);
                bestMove = tiedMoves[dist(rng)];
            } else {
                bestMove = depthBest;
            }

            auto end = std::chrono::high_resolution_clock::now(); // End timer
            long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(); 
            stats.push_back({positionsEvaluated, ms, bestEval}); // Save stats for this depth
        }

        // Print all depth stats (only when debug output is enabled)
        if (g_debugOutput) {
            std::cout << "\n=== Botv1 Search ===" << std::endl;
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

    std::string getName() const override { return "Botv1"; }

private:
    mutable std::mt19937 rng;
    int maxDepth = MAX_DEPTH;
    int positionsEvaluated = 0;

    int alphaBeta(MoveValidator& validator, BitboardEngine& eng, int depth, int currentColor, int alpha, int beta) {
        // Leaf node return static evaluation of the position
        if (depth == 0) {
            positionsEvaluated++;
            return evaluate(eng);
        }

        // Generate all legal moves for the side to move and order them
        std::vector<Move> moves = generateAllMoves(eng, validator, currentColor);

        // no legal moves 
        if (moves.empty()) {
            if (validator.isKingInCheck(currentColor)) {
                // Checkmate
                return (currentColor == 0) ? (-100000 - depth) : (100000 + depth);
            }
            return 0; // Stalemate
        }

        // Order moves for better pruning (captures first via MVV-LVA)
        Move noMove(0,0,0,0);
        orderMoves(moves, eng, noMove);

        if (currentColor == 0) {
            // maximize
            int maxEval = INT_MIN;
            for (auto& move : moves) {
                EngineState engState = saveEngineState(eng);
                MoveValidator::ValidatorState valState = validator.getState();

                Move m = move;
                validator.executeMove(m, currentColor, true);

                int eval = alphaBeta(validator, eng, depth - 1, 1, alpha, beta);

                restoreEngineState(eng, engState);
                validator.setState(valState);

                if (eval > maxEval) maxEval = eval;
                if (eval > alpha) alpha = eval;
                if (alpha >= beta) break;  // Beta cutoff
            }
            return maxEval;
        } else {
            // minimize
            int minEval = INT_MAX;
            for (auto& move : moves) {
                EngineState engState = saveEngineState(eng);
                MoveValidator::ValidatorState valState = validator.getState();

                Move m = move;
                validator.executeMove(m, currentColor, true);

                int eval = alphaBeta(validator, eng, depth - 1, 0, alpha, beta);

                restoreEngineState(eng, engState);
                validator.setState(valState);

                if (eval < minEval) minEval = eval;
                if (eval < beta) beta = eval;
                if (alpha >= beta) break;  // Alpha cutoff
            }
            return minEval;
        }
    }

    // Returns a score for move ordering. Higher = search first
    static int scoreMove(const Move& move, const BitboardEngine& eng, const Move& prevBest) {
        // Previous iteration's best move gets searched first
        if (move.fromRow == prevBest.fromRow && move.fromCol == prevBest.fromCol &&
            move.toRow == prevBest.toRow && move.toCol == prevBest.toCol) {
            return 100000;
        }

        // Promotions are very promising
        if (move.promotedTo != -1) {
            return 50000 + pieceValue(move.promotedTo);
        }

        int score = 0;

        // MVV-LVA: Most Valuable Victim - Least Valuable Attacker
        int captured = eng.getPieceAt(move.toRow, move.toCol);
        if (captured != -1) {
            int attacker = eng.getPieceAt(move.fromRow, move.fromCol);
            // Capture score = victim value * 10 - attacker value (so PxQ >> QxQ >> QxP)
            score += pieceValue(captured) * 10 - pieceValue(attacker);
            score += 10000; // all captures above quiet moves
        }

        return score;
    }

    // Material value for a piece (used by move ordering)
    static int pieceValue(int piece) {
        switch (piece / 2) {
            case 0: return 100;  // pawn
            case 1: return 500;  // rook
            case 2: return 300;  // knight
            case 3: return 300;  // bishop
            case 4: return 900;  // queen
            case 5: return 10000; // king
            default: return 0;
        }
    }

    // Sort moves in descending order of their score
    static void orderMoves(std::vector<Move>& moves, const BitboardEngine& eng, const Move& prevBest) {
        std::sort(moves.begin(), moves.end(), [&](const Move& a, const Move& b) {
            return scoreMove(a, eng, prevBest) > scoreMove(b, eng, prevBest);
        });
    }

    // returns material count from white's perspective (positive = white is ahead, negative = black is ahead)
    static int evaluate(const BitboardEngine& eng) {
        int score = 0;
        score += __builtin_popcountll(eng.pawns[0])   * 100;
        score -= __builtin_popcountll(eng.pawns[1])   * 100;
        score += __builtin_popcountll(eng.knights[0])  * 300;
        score -= __builtin_popcountll(eng.knights[1])  * 300;
        score += __builtin_popcountll(eng.bishops[0])  * 300;
        score -= __builtin_popcountll(eng.bishops[1])  * 300;
        score += __builtin_popcountll(eng.rooks[0])    * 500;
        score -= __builtin_popcountll(eng.rooks[1])    * 500;
        score += __builtin_popcountll(eng.queens[0])   * 900;
        score -= __builtin_popcountll(eng.queens[1])   * 900;
        // Kings always present, so no need to count them
        return score;
    }

    // Generate all legal moves for a given position and color
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
                        // Expand into 4 promotion choices
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

    // holds all bitboards
    struct EngineState {
        Bitboard pawns[2], rooks[2], knights[2], bishops[2], queens[2], kings[2];
        Bitboard allWhitePieces, allBlackPieces, allPieces;
    };

    // returns a snapshot of the entire engine state
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

    // restores a previously saved engine state
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
