#pragma once

#include <cstdint>
#include <string>

// Bitboard type for representing piece positions
using Bitboard = uint64_t;

class BitboardEngine {
public:
    BitboardEngine();
    ~BitboardEngine();
    
    // Bitboards for each piece type (12 total: 6 piece types × 2 colors)
    Bitboard pawns[2];      // 0 = white, 1 = black
    Bitboard rooks[2];
    Bitboard knights[2];
    Bitboard bishops[2];
    Bitboard queens[2];
    Bitboard kings[2];
    
    // Combined bitboards
    Bitboard allWhitePieces;
    Bitboard allBlackPieces;
    Bitboard allPieces;
    
    // Initialize to starting position
    void initializeStartingPosition();
    
    // Get piece at square (row, col)
    int getPieceAt(int row, int col) const;
    
    // Set piece at square
    void setPieceAt(int row, int col, int piece);
    
    // Clear piece at square
    void clearSquare(int row, int col);
    
    // Move piece from one square to another
    void movePiece(int fromRow, int fromCol, int toRow, int toCol);
    
    // Get bitboard index from row and column (0-63)
    static int squareToIndex(int row, int col);
    
    // Get row and column from bitboard index
    static void indexToSquare(int index, int& row, int& col);
    
    // Convert square coordinates to algebraic notation (e.g., "a1", "h8")
    static std::string squareToAlgebraic(int row, int col);
    
    // Get piece character for logging ('P', 'R', 'N', 'B', 'Q', 'K')
    static char getPieceChar(int piece);
    
    // Print current board state to console
    void printBoard() const;
    
    // Update combined bitboards after manual bitboard changes
    void updateCombinedBitboards();
    
    // Piece type constants (public for use by bots and validators)
    static const int WHITE_PAWN = 0;
    static const int BLACK_PAWN = 1;
    static const int WHITE_ROOK = 2;
    static const int BLACK_ROOK = 3;
    static const int WHITE_KNIGHT = 4;
    static const int BLACK_KNIGHT = 5;
    static const int WHITE_BISHOP = 6;
    static const int BLACK_BISHOP = 7;
    static const int WHITE_QUEEN = 8;
    static const int BLACK_QUEEN = 9;
    static const int WHITE_KING = 10;
    static const int BLACK_KING = 11;
    static const int EMPTY = -1;
};
