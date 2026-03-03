#pragma once

#include "BitboardEngine.h"
#include <cstdint>

// PeSTO-based evaluation with tapered eval, pawn structure, and mobility.
namespace Eval {

// Material values (using PeSTO values)
static constexpr int MG_PAWN_VAL   =  82;
static constexpr int MG_KNIGHT_VAL = 337;
static constexpr int MG_BISHOP_VAL = 365;
static constexpr int MG_ROOK_VAL   = 477;
static constexpr int MG_QUEEN_VAL  = 1025;

static constexpr int EG_PAWN_VAL   =  94;
static constexpr int EG_KNIGHT_VAL = 281;
static constexpr int EG_BISHOP_VAL = 297;
static constexpr int EG_ROOK_VAL   = 512;
static constexpr int EG_QUEEN_VAL  = 936;

// Game phase weights for tapered eval (total starting phase = 24)
static constexpr int KNIGHT_PHASE = 1;
static constexpr int BISHOP_PHASE = 1;
static constexpr int ROOK_PHASE   = 2;
static constexpr int QUEEN_PHASE  = 4;
static constexpr int TOTAL_PHASE  = 24; // 4*1 + 4*1 + 4*2 + 2*4

// PeSTO Piece-Square Tables
// Indexed [square], where square 0 = a1, 1 = b1, ..., 63 = h8

// Middlegame tables
static constexpr int MG_PAWN_TABLE[64] = {
      0,   0,   0,   0,   0,   0,  0,   0,
     98, 134,  61,  95,  68, 126, 34, -11,
     -6,   7,  26,  31,  65,  56, 25, -20,
    -14,  13,   6,  21,  23,  12, 17, -23,
    -27,  -2,  -5,  12,  17,   6, 10, -25,
    -26,  -4,  -4, -10,   3,   3, 33, -12,
    -35,  -1, -20, -23, -15,  24, 38, -22,
      0,   0,   0,   0,   0,   0,  0,   0,
};

static constexpr int MG_KNIGHT_TABLE[64] = {
    -167, -89, -34, -49,  61, -97, -15, -107,
     -73, -41,  72,  36,  23,  62,   7,  -17,
     -47,  60,  37,  65,  84, 129,  73,   44,
      -9,  17,  19,  53,  37,  69,  18,   22,
     -13,   4,  16,  13,  28,  19,  21,   -8,
     -23,  -9,  12,  10,  19,  17,  25,  -16,
     -29, -53, -12,  -3,  -1,  18, -14,  -19,
    -105, -21, -58, -33, -17, -28, -19,  -23,
};

static constexpr int MG_BISHOP_TABLE[64] = {
    -29,   4, -82, -37, -25, -42,   7,  -8,
    -26,  16, -18, -13,  30,  59,  18, -47,
    -16,  37,  43,  40,  35,  50,  37,  -2,
     -4,   5,  19,  50,  37,  37,   7,  -2,
     -6,  13,  13,  26,  34,  12,  10,   4,
      0,  15,  15,  15,  14,  27,  18,  10,
      4,  15,  16,   0,   7,  21,  33,   1,
    -33,  -3, -14, -21, -13, -12, -39, -21,
};

static constexpr int MG_ROOK_TABLE[64] = {
     32,  42,  32,  51, 63,  9,  31,  43,
     27,  32,  58,  62, 80, 67,  26,  44,
     -5,  19,  26,  36, 17, 45,  61,  16,
    -24, -11,   7,  26, 24, 35,  -8, -20,
    -36, -26, -12,  -1,  9, -7,   6, -23,
    -45, -25, -16, -17,  3,  0,  -5, -33,
    -44, -16, -20,  -9, -1, 11,  -6, -71,
    -19, -13,   1,  17, 16,  7, -37, -26,
};

static constexpr int MG_QUEEN_TABLE[64] = {
    -28,   0,  29,  12,  59,  44,  43,  45,
    -24, -39,  -5,   1, -16,  57,  28,  54,
    -13, -17,   7,   8,  29,  56,  47,  57,
    -27, -27, -16, -16,  -1,  17,  -2,   1,
     -9, -26,  -9, -10,  -2,  -4,   3,  -3,
    -14,   2, -11,  -2,  -5,   2,  14,   5,
    -35,  -8,  11,   2,   8,  15,  -3,   1,
     -1, -18,  -9,  10, -15, -25, -31, -50,
};

static constexpr int MG_KING_TABLE[64] = {
    -65,  23,  16, -15, -56, -34,   2,  13,
     29,  -1, -20,  -7,  -8,  -4, -38, -29,
     -9,  24,   2, -16, -20,   6,  22, -22,
    -17, -20, -12, -27, -30, -25, -14, -36,
    -49,  -1, -27, -39, -46, -44, -33, -51,
    -14, -14, -22, -46, -44, -30, -15, -27,
      1,   7,  -8, -64, -43, -16,   9,   8,
    -15,  36,  12, -54,   8, -28,  24,  14,
};

// Endgame tables
static constexpr int EG_PAWN_TABLE[64] = {
      0,   0,   0,   0,   0,   0,   0,   0,
    178, 173, 158, 134, 147, 132, 165, 187,
     94, 100,  85,  67,  56,  53,  82,  84,
     32,  24,  13,   5,  -2,   4,  17,  17,
     13,   9,  -3,  -7,  -7,  -8,   3,  -1,
      4,   7,  -6,   1,   0,  -5,  -1,  -8,
     13,   8,   8,  10,  13,   0,   2,  -7,
      0,   0,   0,   0,   0,   0,   0,   0,
};

static constexpr int EG_KNIGHT_TABLE[64] = {
    -58, -38, -13, -28, -31, -27, -63, -99,
    -25,  -8, -25,  -2,  -9, -25, -24, -52,
    -24, -20,  10,   9,  -1,  -9, -19, -41,
    -17,   3,  22,  22,  22,  11,   8, -18,
    -18,  -6,  16,  25,  16,  17,   4, -18,
    -23,  -3,  -1,  15,  10,  -3, -20, -22,
    -42, -20, -10,  -5,  -2, -20, -23, -44,
    -29, -51, -23, -15, -22, -18, -50, -64,
};

static constexpr int EG_BISHOP_TABLE[64] = {
    -14, -21, -11,  -8, -7,  -9, -17, -24,
     -8,  -4,   7, -12, -3, -13,  -4, -14,
      2,  -8,   0,  -1, -2,   6,   0,   4,
     -3,   9,  12,   9, 14,  10,   3,   2,
     -6,   3,  13,  19,  7,  10,  -3,  -9,
    -12,  -3,   8,  10, 13,   3,  -7, -15,
    -14, -18,  -7,  -1,  4,  -9, -15, -27,
    -23,  -9, -23,  -5, -9, -16,  -5, -17,
};

static constexpr int EG_ROOK_TABLE[64] = {
     13, 10, 18, 15, 12,  12,   8,   5,
     11, 13, 13, 11, -3,   3,   8,   3,
      7,  7,  7,  5,  4,  -3,  -5,  -3,
      4,  3, 13,  1,  2,   1,  -1,   2,
      3,  5,  8,  4, -5,  -6,  -8, -11,
     -4,  0, -5, -1, -7, -12,  -8, -16,
     -6, -6,  0,  2, -9,  -9, -11,  -3,
     -9,  2,  3, -1, -5, -13,   4, -20,
};

static constexpr int EG_QUEEN_TABLE[64] = {
     -9,  22,  22,  27,  27,  19,  10,  20,
    -17,  20,  32,  41,  58,  25,  30,   0,
    -20,   6,   9,  49,  47,  35,  19,   9,
      3,  22,  24,  45,  57,  40,  57,  36,
    -18,  28,  19,  47,  31,  34,  39,  23,
    -16, -27,  15,   6,   9,  17,  10,   5,
    -22, -23, -30, -16, -16, -23, -36, -32,
    -33, -28, -22, -43,  -5, -32, -20, -41,
};

static constexpr int EG_KING_TABLE[64] = {
    -74, -35, -18, -18, -11,  15,   4, -17,
    -12,  17,  14,  17,  17,  38,  23,  11,
     10,  17,  23,  15,  20,  45,  44,  13,
     -8,  22,  24,  27,  26,  33,  26,   3,
    -18,  -4,  21,  24,  27,  23,   9, -11,
    -19,  -3,  11,  21,  23,  16,   7,  -9,
    -27, -11,   4,  13,  14,   4,  -5, -17,
    -53, -34, -21, -11, -28, -14, -24, -43,
};

// Pawn structure bonuses/penalties
static constexpr int DOUBLED_PAWN_PENALTY   = -10;
static constexpr int ISOLATED_PAWN_PENALTY  = -20;
static constexpr int BACKWARD_PAWN_PENALTY  = -10;
static constexpr int PASSED_PAWN_BONUS[8]   = { 0, 10, 20, 40, 60, 90, 130, 0 };
// Index by rank distance from promotion: rank 0 or 7 are promo ranks

// Mobility weight
static constexpr int MOBILITY_MG = 3;
static constexpr int MOBILITY_EG = 2;

// Bishop pair bonus
static constexpr int BISHOP_PAIR_MG = 30;
static constexpr int BISHOP_PAIR_EG = 50;

// Maps (row, col) to piece-square table index for white and black perspective
inline int whitePstIndex(int row, int col) { return row * 8 + col; }
inline int blackPstIndex(int row, int col) { return (7 - row) * 8 + col; }

// Compute game phase (TOTAL_PHASE = opening with all pieces, 0 = endgame)
inline int computePhase(const BitboardEngine& eng) {
    int phase = 0;
    phase += __builtin_popcountll(eng.knights[0]) * KNIGHT_PHASE;
    phase += __builtin_popcountll(eng.knights[1]) * KNIGHT_PHASE;
    phase += __builtin_popcountll(eng.bishops[0]) * BISHOP_PHASE;
    phase += __builtin_popcountll(eng.bishops[1]) * BISHOP_PHASE;
    phase += __builtin_popcountll(eng.rooks[0])   * ROOK_PHASE;
    phase += __builtin_popcountll(eng.rooks[1])   * ROOK_PHASE;
    phase += __builtin_popcountll(eng.queens[0])  * QUEEN_PHASE;
    phase += __builtin_popcountll(eng.queens[1])  * QUEEN_PHASE;
    if (phase > TOTAL_PHASE) phase = TOTAL_PHASE; // cap in case of promotions
    return phase;
}

// Sum PST values for all pieces of given bitboard.
inline void addPstScores(Bitboard bb, const int* mgTable, const int* egTable,
                         int (*pstFunc)(int, int), int& mgScore, int& egScore) {
    while (bb) {
        int idx = __builtin_ctzll(bb);  // lowest set bit index
        int row = idx / 8;
        int col = idx % 8;
        int pstIdx = pstFunc(row, col);
        mgScore += mgTable[pstIdx];
        egScore += egTable[pstIdx];
        bb &= bb - 1;  // clear lowest bit
    }
}

// Pawn structure evaluation 
inline void evalPawnStructure(Bitboard ownPawns, Bitboard enemyPawns, int color,
                              int& mgBonus, int& egBonus) {
    mgBonus = 0;
    egBonus = 0;

    // Precompute pawn file counts for file-based analysis
    int fileCounts[8] = {};
    Bitboard tmp = ownPawns;
    while (tmp) {
        int idx = __builtin_ctzll(tmp);
        int col = idx % 8;
        fileCounts[col]++;
        tmp &= tmp - 1;
    }

    // Penalize files with more than 1 pawn (doubled pawns)
    for (int f = 0; f < 8; f++) {
        if (fileCounts[f] > 1) {
            int extra = fileCounts[f] - 1;
            mgBonus += extra * DOUBLED_PAWN_PENALTY;
            egBonus += extra * DOUBLED_PAWN_PENALTY;
        }
    }

    // Look at each pawn for isolated, backward, or passed status
    tmp = ownPawns;
    while (tmp) {
        int idx = __builtin_ctzll(tmp);
        int row = idx / 8;
        int col = idx % 8;

        // Build adjacent file mask for this pawn
        bool hasAdjacentFriendly = false;
        if (col > 0 && fileCounts[col - 1] > 0) hasAdjacentFriendly = true;
        if (col < 7 && fileCounts[col + 1] > 0) hasAdjacentFriendly = true;

        // Isolated pawn
        if (!hasAdjacentFriendly) {
            mgBonus += ISOLATED_PAWN_PENALTY;
            egBonus += ISOLATED_PAWN_PENALTY;
        }

        // Passed pawn
        bool passed = true;
        Bitboard enemyTmp = enemyPawns;
        while (enemyTmp) {
            int eIdx = __builtin_ctzll(enemyTmp);
            int eRow = eIdx / 8;
            int eCol = eIdx % 8;
            if (eCol >= col - 1 && eCol <= col + 1) {
                if (color == 0 && eRow <= row) { passed = false; break; }
                if (color == 1 && eRow >= row) { passed = false; break; }
            }
            enemyTmp &= enemyTmp - 1;
        }

        if (passed) {
            // Rank distance from promotion for passed pawn
            int rankFromPromo = (color == 0) ? row : (7 - row);
            if (rankFromPromo >= 1 && rankFromPromo <= 6) {
                int bonus = PASSED_PAWN_BONUS[7 - rankFromPromo];
                mgBonus += bonus / 2;   // passed pawns more valuable in endgame
                egBonus += bonus;
            }
        }

        // Backward pawn
        if (hasAdjacentFriendly) {
            bool hasSupport = false;
            Bitboard friendTmp = ownPawns;
            while (friendTmp) {
                int fIdx = __builtin_ctzll(friendTmp);
                int fRow = fIdx / 8;
                int fCol = fIdx % 8;
                if (fCol == col - 1 || fCol == col + 1) {
                    // "behind or equal" depends on color
                    if (color == 0 && fRow >= row) { hasSupport = true; break; }
                    if (color == 1 && fRow <= row) { hasSupport = true; break; }
                }
                friendTmp &= friendTmp - 1;
            }
            if (!hasSupport) {
                mgBonus += BACKWARD_PAWN_PENALTY;
                egBonus += BACKWARD_PAWN_PENALTY;
            }
        }

        tmp &= tmp - 1;
    }
}

// Full evaluation function using PeSTO PSTs + tapered eval + pawn structure + mobility
inline int evaluate(const BitboardEngine& eng, int mobilityWhite = 0, int mobilityBlack = 0) {
    int mgScore = 0, egScore = 0;

    // White pieces (add)
    int wPawns   = __builtin_popcountll(eng.pawns[0]);
    int wKnights = __builtin_popcountll(eng.knights[0]);
    int wBishops = __builtin_popcountll(eng.bishops[0]);
    int wRooks   = __builtin_popcountll(eng.rooks[0]);
    int wQueens  = __builtin_popcountll(eng.queens[0]);

    mgScore += wPawns   * MG_PAWN_VAL;
    mgScore += wKnights * MG_KNIGHT_VAL;
    mgScore += wBishops * MG_BISHOP_VAL;
    mgScore += wRooks   * MG_ROOK_VAL;
    mgScore += wQueens  * MG_QUEEN_VAL;

    egScore += wPawns   * EG_PAWN_VAL;
    egScore += wKnights * EG_KNIGHT_VAL;
    egScore += wBishops * EG_BISHOP_VAL;
    egScore += wRooks   * EG_ROOK_VAL;
    egScore += wQueens  * EG_QUEEN_VAL;

    // Black pieces (subtract)
    int bPawns   = __builtin_popcountll(eng.pawns[1]);
    int bKnights = __builtin_popcountll(eng.knights[1]);
    int bBishops = __builtin_popcountll(eng.bishops[1]);
    int bRooks   = __builtin_popcountll(eng.rooks[1]);
    int bQueens  = __builtin_popcountll(eng.queens[1]);

    mgScore -= bPawns   * MG_PAWN_VAL;
    mgScore -= bKnights * MG_KNIGHT_VAL;
    mgScore -= bBishops * MG_BISHOP_VAL;
    mgScore -= bRooks   * MG_ROOK_VAL;
    mgScore -= bQueens  * MG_QUEEN_VAL;

    egScore -= bPawns   * EG_PAWN_VAL;
    egScore -= bKnights * EG_KNIGHT_VAL;
    egScore -= bBishops * EG_BISHOP_VAL;
    egScore -= bRooks   * EG_ROOK_VAL;
    egScore -= bQueens  * EG_QUEEN_VAL;

    // PST bonuses white pieces
    addPstScores(eng.pawns[0],   MG_PAWN_TABLE,   EG_PAWN_TABLE,   whitePstIndex, mgScore, egScore);
    addPstScores(eng.knights[0], MG_KNIGHT_TABLE, EG_KNIGHT_TABLE, whitePstIndex, mgScore, egScore);
    addPstScores(eng.bishops[0], MG_BISHOP_TABLE, EG_BISHOP_TABLE, whitePstIndex, mgScore, egScore);
    addPstScores(eng.rooks[0],   MG_ROOK_TABLE,   EG_ROOK_TABLE,   whitePstIndex, mgScore, egScore);
    addPstScores(eng.queens[0],  MG_QUEEN_TABLE,  EG_QUEEN_TABLE,  whitePstIndex, mgScore, egScore);
    addPstScores(eng.kings[0],   MG_KING_TABLE,   EG_KING_TABLE,   whitePstIndex, mgScore, egScore);

    // PST bonuses black pieces (subtract)
    int bMg = 0, bEg = 0;
    addPstScores(eng.pawns[1],   MG_PAWN_TABLE,   EG_PAWN_TABLE,   blackPstIndex, bMg, bEg);
    addPstScores(eng.knights[1], MG_KNIGHT_TABLE, EG_KNIGHT_TABLE, blackPstIndex, bMg, bEg);
    addPstScores(eng.bishops[1], MG_BISHOP_TABLE, EG_BISHOP_TABLE, blackPstIndex, bMg, bEg);
    addPstScores(eng.rooks[1],   MG_ROOK_TABLE,   EG_ROOK_TABLE,   blackPstIndex, bMg, bEg);
    addPstScores(eng.queens[1],  MG_QUEEN_TABLE,  EG_QUEEN_TABLE,  blackPstIndex, bMg, bEg);
    addPstScores(eng.kings[1],   MG_KING_TABLE,   EG_KING_TABLE,   blackPstIndex, bMg, bEg);
    mgScore -= bMg;
    egScore -= bEg;

    // Bishop pair bonus
    if (wBishops >= 2) { mgScore += BISHOP_PAIR_MG; egScore += BISHOP_PAIR_EG; }
    if (bBishops >= 2) { mgScore -= BISHOP_PAIR_MG; egScore -= BISHOP_PAIR_EG; }

    // Pawn structure
    int wPawnMg = 0, wPawnEg = 0;
    evalPawnStructure(eng.pawns[0], eng.pawns[1], 0, wPawnMg, wPawnEg);
    mgScore += wPawnMg;
    egScore += wPawnEg;

    int bPawnMg = 0, bPawnEg = 0;
    evalPawnStructure(eng.pawns[1], eng.pawns[0], 1, bPawnMg, bPawnEg);
    mgScore -= bPawnMg;
    egScore -= bPawnEg;

    // Mobility
    mgScore += (mobilityWhite - mobilityBlack) * MOBILITY_MG;
    egScore += (mobilityWhite - mobilityBlack) * MOBILITY_EG;

    // Tapered eval
    int phase = computePhase(eng);
    // Interpolated score
    int score = (mgScore * phase + egScore * (TOTAL_PHASE - phase)) / TOTAL_PHASE;

    return score;
}

}
