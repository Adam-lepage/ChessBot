#pragma once

#include "ChessBot.h"
#include <vector>
#include <random>
#include <string>

// A simple built-in bot that picks a random legal move.
// Useful for testing and as a default opponent.
class RandomBot : public ChessBot {
public:
    RandomBot() : rng(std::random_device{}()) {}

    Move chooseMove(const BitboardEngine& engine, MoveValidator& validator, int color) override {
        // Collect all legal moves for this color
        std::vector<Move> allMoves;

        for (int row = 0; row < 8; row++) {
            for (int col = 0; col < 8; col++) {
                int piece = engine.getPieceAt(row, col);
                if (piece == -1) continue;
                int pieceColor = (piece % 2 == 0) ? 0 : 1;
                if (pieceColor != color) continue;

                auto moves = validator.getValidMoves(row, col, color);
                allMoves.insert(allMoves.end(), moves.begin(), moves.end());
            }
        }

        if (allMoves.empty()) {
            // No moves available (shouldn't happen if called correctly)
            return Move(0, 0, 0, 0);
        }

        std::uniform_int_distribution<size_t> dist(0, allMoves.size() - 1);
        Move chosen = allMoves[dist(rng)];
        
        // Always promote to queen
        int piece = engine.getPieceAt(chosen.fromRow, chosen.fromCol);
        if (piece / 2 == 0) {  // It's a pawn
            if ((color == 0 && chosen.toRow == 0) || (color == 1 && chosen.toRow == 7)) {
                chosen.promotedTo = (color == 0) ? BitboardEngine::WHITE_QUEEN : BitboardEngine::BLACK_QUEEN;
            }
        }
        
        return chosen;
    }

    std::string getName() const override { return "RandomBot"; }

private:
    std::mt19937 rng;
};
