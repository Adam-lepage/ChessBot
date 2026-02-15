#pragma once

#include "BitboardEngine.h"
#include <vector>
#include <string>

struct Move {
    int fromRow, fromCol;
    int toRow, toCol;
    int capturedPiece;  // -1 if no capture
    bool isEnPassant;
    bool isPawnPromotion;
    int promotedTo;  // Piece type to promote to
    bool isCastling;  // True if this move is a castling move
    
    Move(int fr, int fc, int tr, int tc) 
        : fromRow(fr), fromCol(fc), toRow(tr), toCol(tc), 
          capturedPiece(-1), isEnPassant(false), isPawnPromotion(false), promotedTo(-1),
          isCastling(false) {}
};

class MoveValidator {
public:
    MoveValidator(BitboardEngine* engine);
    ~MoveValidator();
    
    // Check if a move is valid
    bool isValidMove(int fromRow, int fromCol, int toRow, int toCol, int playerColor);
    
    // Get all valid moves for a piece
    std::vector<Move> getValidMoves(int row, int col, int playerColor);
    
    // Execute a move (updates bitboard and handles captures/en passant)
    // Populates move flags (isEnPassant, isPawnPromotion, capturedPiece)
    bool executeMove(Move& move, int playerColor);
    
    // Check if a king is in check
    bool isKingInCheck(int playerColor);
    
    // Check if player is in checkmate
    bool isCheckmate(int playerColor);
    
    // Check if player is in stalemate (no legal moves, not in check)
    bool isStalemate(int playerColor);
    
    // Check if the player has any legal moves at all
    bool hasAnyLegalMoves(int playerColor);
    
    // Check if position is attacked by enemy
    bool isSquareAttacked(int row, int col, int byColor);
    
    // Get the square of last double pawn push (for en passant)
    void setLastEnPassantSquare(int row, int col) { lastEnPassantRow = row; lastEnPassantCol = col; }
    void clearEnPassantSquare() { lastEnPassantRow = -1; lastEnPassantCol = -1; }
    
    // Get piece at square
    int getPieceAt(int row, int col) const { return engine->getPieceAt(row, col); }
    
    // Castling rights
    void resetCastlingRights();
    bool canCastleKingside(int playerColor) const;
    bool canCastleQueenside(int playerColor) const;

private:
    BitboardEngine* engine;
    int lastEnPassantRow, lastEnPassantCol;
    
    // Castling rights tracking
    bool whiteKingsideCastle;
    bool whiteQueensideCastle;
    bool blackKingsideCastle;
    bool blackQueensideCastle;
    
    // Helper to get bitboard pointer for a piece ID
    Bitboard* getBitboardForPiece(int piece);
    
    // Castling helpers
    bool isCastlingMove(int fromRow, int fromCol, int toRow, int toCol, int playerColor);
    void updateCastlingRights(int piece, int fromRow, int fromCol);
    
    // Piece constants for easier use
    static const int WHITE = 0;
    static const int BLACK = 1;
    
    // Helper functions for move validation
    bool isPawnMove(int piece, int fromRow, int fromCol, int toRow, int toCol, int playerColor);
    bool isRookMove(int fromRow, int fromCol, int toRow, int toCol);
    bool isKnightMove(int fromRow, int fromCol, int toRow, int toCol);
    bool isBishopMove(int fromRow, int fromCol, int toRow, int toCol);
    bool isQueenMove(int fromRow, int fromCol, int toRow, int toCol);
    bool isKingMove(int fromRow, int fromCol, int toRow, int toCol);
    
    // Path checking
    bool isPathClear(int fromRow, int fromCol, int toRow, int toCol);
    bool isDiagonalPathClear(int fromRow, int fromCol, int toRow, int toCol);
    bool isHorizontalPathClear(int fromRow, int fromCol, int toRow, int toCol);
    bool isVerticalPathClear(int fromRow, int fromCol, int toRow, int toCol);
};
