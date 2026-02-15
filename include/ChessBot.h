#pragma once

#include "BitboardEngine.h"
#include "MoveValidator.h"
#include <string>

class ChessBot {
public:
    virtual ~ChessBot() = default;

    // Called each turn to get the bot's chosen move.
    // engine  - read-only view of current piece positions (bitboards)
    // validator - use to query valid moves and check game state
    // color   - 0 = white, 1 = black (matches BitboardEngine constants)
    virtual Move chooseMove(const BitboardEngine& engine, MoveValidator& validator, int color) = 0;

    // Return a display name for this bot (used in logs)
    virtual std::string getName() const = 0;
};
