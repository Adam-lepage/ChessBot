#pragma once

#include "ChessBot.h"
#include <vector>
#include <string>
#include <iostream>
#include <chrono>
#include <climits>

class Botv1 : public ChessBot {
public:
    static constexpr int MAX_DEPTH = 5;

    Move chooseMove(const BitboardEngine&, MoveValidator& validator, int color) override {
        BitboardEngine* eng = validator.getEngine();

        // Generate all legal moves at the root
        std::vector<Move> rootMoves = generateAllMoves(*eng, validator, color);
        if (rootMoves.empty()) return Move(0, 0, 0, 0);

        Move bestMove = rootMoves[0];

        // Stats per depth level
        struct DepthStats { int positions; long long timeMs; int eval; };
        std::vector<DepthStats> stats;

        // Suppress cout during search
        std::streambuf* coutBuf = std::cout.rdbuf(nullptr);

        // Iterative deepening with minimax
        for (int depth = 1; depth <= MAX_DEPTH; depth++) {
            auto start = std::chrono::high_resolution_clock::now(); // Start timer
            positionsEvaluated = 0;

            int bestEval = (color == 0) ? INT_MIN : INT_MAX;
            Move depthBest = rootMoves[0];

            // For each root move, execute it, then call minimax for opponent's reply
            for (auto& rootMove : rootMoves) {
                // Save full state before making the move
                EngineState engState = saveEngineState(*eng);
                MoveValidator::ValidatorState valState = validator.getState();

                // Execute move
                Move m = rootMove;
                validator.executeMove(m, color);

                // Recurse into opponent's reply
                int eval = minimax(validator, *eng, depth - 1, 1 - color);

                // Restore state
                restoreEngineState(*eng, engState);
                validator.setState(valState);

                // White maximizes, black minimizes
                if (color == 0) {
                    if (eval > bestEval) { bestEval = eval; depthBest = rootMove; }
                } else {
                    if (eval < bestEval) { bestEval = eval; depthBest = rootMove; }
                }
            }

            bestMove = depthBest;

            auto end = std::chrono::high_resolution_clock::now(); // End timer
            long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(); 
            stats.push_back({positionsEvaluated, ms, bestEval}); // Save stats for this depth
        }

        // Print all depth stats
        std::cout.rdbuf(coutBuf);

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

        return bestMove;
    }

    std::string getName() const override { return "Botv1"; }

private:
    int positionsEvaluated = 0;

    int minimax(MoveValidator& validator, BitboardEngine& eng, int depth, int currentColor) {
        // Leaf node — return static material evaluation
        if (depth == 0) {
            positionsEvaluated++;
            return evaluate(eng);
        }

        // Generate all legal moves for the side to move
        std::vector<Move> moves = generateAllMoves(eng, validator, currentColor);

        // no legal moves 
        if (moves.empty()) {
            if (validator.isKingInCheck(currentColor)) {
                // Checkmate
                return (currentColor == 0) ? (-100000 - depth) : (100000 + depth);
            }
            return 0; // Stalemate
        }

        if (currentColor == 0) {
            // maximize
            int maxEval = INT_MIN;
            // For each move, execute it, call minimax for opponent's reply, then undo the move
            for (auto& move : moves) {
                EngineState engState = saveEngineState(eng);
                MoveValidator::ValidatorState valState = validator.getState();

                Move m = move;
                validator.executeMove(m, currentColor);

                int eval = minimax(validator, eng, depth - 1, 1);

                restoreEngineState(eng, engState);
                validator.setState(valState);

                if (eval > maxEval) maxEval = eval;
            }
            return maxEval;
        } else {
            // minimize
            int minEval = INT_MAX;
            for (auto& move : moves) {
                EngineState engState = saveEngineState(eng);
                MoveValidator::ValidatorState valState = validator.getState();

                Move m = move;
                validator.executeMove(m, currentColor);

                int eval = minimax(validator, eng, depth - 1, 0);

                restoreEngineState(eng, engState);
                validator.setState(valState);

                if (eval < minEval) minEval = eval;
            }
            return minEval;
        }
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
