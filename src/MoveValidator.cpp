#include "MoveValidator.h"
#include <iostream>
#include <cmath>
#include <cstdint>

using Bitboard = uint64_t;

MoveValidator::MoveValidator(BitboardEngine* engine) 
    : engine(engine), lastEnPassantRow(-1), lastEnPassantCol(-1) {
}

MoveValidator::~MoveValidator() {
}

bool MoveValidator::isValidMove(int fromRow, int fromCol, int toRow, int toCol, int playerColor) {
    // Check bounds
    if (fromRow < 0 || fromRow > 7 || fromCol < 0 || fromCol > 7 ||
        toRow < 0 || toRow > 7 || toCol < 0 || toCol > 7) {
        return false;
    }
    
    // Can't move to the same square
    if (fromRow == toRow && fromCol == toCol) return false;
    
    int piece = getPieceAt(fromRow, fromCol);
    int targetPiece = getPieceAt(toRow, toCol);
    
    // No piece at source
    if (piece == -1) return false;
    
    // Piece doesn't belong to player
    int pieceColor = (piece % 2 == 0) ? WHITE : BLACK;
    if (pieceColor != playerColor) return false;
    
    // Can't capture own piece
    if (targetPiece != -1) {
        int targetColor = (targetPiece % 2 == 0) ? WHITE : BLACK;
        if (targetColor == playerColor) return false;
    }
    
    // Get the base piece type (0-5)
    int basePiece = piece / 2;
    
    // Validate move based on piece type
    switch (basePiece) {
        case 0:  // Pawn
            if (!isPawnMove(piece, fromRow, fromCol, toRow, toCol, playerColor)) return false;
            break;
        case 1:  // Rook
            if (!isRookMove(fromRow, fromCol, toRow, toCol)) return false;
            if (!isPathClear(fromRow, fromCol, toRow, toCol)) return false;
            break;
        case 2:  // Knight
            if (!isKnightMove(fromRow, fromCol, toRow, toCol)) return false;
            break;
        case 3:  // Bishop
            if (!isBishopMove(fromRow, fromCol, toRow, toCol)) return false;
            if (!isPathClear(fromRow, fromCol, toRow, toCol)) return false;
            break;
        case 4:  // Queen
            if (!isQueenMove(fromRow, fromCol, toRow, toCol)) return false;
            if (!isPathClear(fromRow, fromCol, toRow, toCol)) return false;
            break;
        case 5:  // King
            if (!isKingMove(fromRow, fromCol, toRow, toCol)) return false;
            break;
        default:
            return false;
    }
    
    // CRITICAL: Check if move would leave king in check
    // We must simulate the move and check if the king would be in check
    
    int fromIndex = fromRow * 8 + fromCol;
    int toIndex = toRow * 8 + toCol;
    
    // Get pointers to source and target bitboards
    Bitboard* sourceBitboard = nullptr;
    Bitboard* targetBitboard = nullptr;
    
    // Map piece IDs to bitboard pointers
    if (piece == 0) sourceBitboard = &engine->pawns[0];
    else if (piece == 1) sourceBitboard = &engine->pawns[1];
    else if (piece == 2) sourceBitboard = &engine->rooks[0];
    else if (piece == 3) sourceBitboard = &engine->rooks[1];
    else if (piece == 4) sourceBitboard = &engine->knights[0];
    else if (piece == 5) sourceBitboard = &engine->knights[1];
    else if (piece == 6) sourceBitboard = &engine->bishops[0];
    else if (piece == 7) sourceBitboard = &engine->bishops[1];
    else if (piece == 8) sourceBitboard = &engine->queens[0];
    else if (piece == 9) sourceBitboard = &engine->queens[1];
    else if (piece == 10) sourceBitboard = &engine->kings[0];
    else if (piece == 11) sourceBitboard = &engine->kings[1];
    
    if (!sourceBitboard) return false;
    
    // Get target bitboard if capturing
    if (targetPiece != -1) {
        if (targetPiece == 0) targetBitboard = &engine->pawns[0];
        else if (targetPiece == 1) targetBitboard = &engine->pawns[1];
        else if (targetPiece == 2) targetBitboard = &engine->rooks[0];
        else if (targetPiece == 3) targetBitboard = &engine->rooks[1];
        else if (targetPiece == 4) targetBitboard = &engine->knights[0];
        else if (targetPiece == 5) targetBitboard = &engine->knights[1];
        else if (targetPiece == 6) targetBitboard = &engine->bishops[0];
        else if (targetPiece == 7) targetBitboard = &engine->bishops[1];
        else if (targetPiece == 8) targetBitboard = &engine->queens[0];
        else if (targetPiece == 9) targetBitboard = &engine->queens[1];
        else if (targetPiece == 10) targetBitboard = &engine->kings[0];
        else if (targetPiece == 11) targetBitboard = &engine->kings[1];
    }
    
    // Save old state
    Bitboard oldSourceBB = *sourceBitboard;
    Bitboard oldTargetBB = 0;
    bool hadTarget = (targetBitboard != nullptr);
    if (hadTarget) {
        oldTargetBB = *targetBitboard;
    }
    
    // Simulate the move
    *sourceBitboard &= ~(1ULL << fromIndex);  // Remove from source
    *sourceBitboard |= (1ULL << toIndex);     // Add to destination
    
    if (targetBitboard) {
        *targetBitboard &= ~(1ULL << toIndex);  // Remove captured piece
    }
    
    // Check if king is in check after this move
    bool kingInCheck = isKingInCheck(playerColor);
    
    // Restore the old state
    *sourceBitboard = oldSourceBB;
    if (hadTarget) {
        *targetBitboard = oldTargetBB;
    }
    
    // If king would be in check, move is illegal
    if (kingInCheck) {
        return false;
    }
    
    return true;
}

bool MoveValidator::isPawnMove(int piece, int fromRow, int fromCol, int toRow, int toCol, int playerColor) {
    int direction = (playerColor == WHITE) ? -1 : 1;  // White moves up (-row), black moves down (+row)
    int expectedRow = fromRow + direction;
    
    // Forward move
    if (toCol == fromCol && toRow == expectedRow && getPieceAt(toRow, toCol) == -1) {
        return true;
    }
    
    // Double pawn push from starting position
    if ((playerColor == WHITE && fromRow == 6) || (playerColor == BLACK && fromRow == 1)) {
        int doublePushRow = fromRow + (2 * direction);
        if (toCol == fromCol && toRow == doublePushRow && 
            getPieceAt(fromRow + direction, fromCol) == -1 && 
            getPieceAt(toRow, toCol) == -1) {
            return true;
        }
    }
    
    // Capture diagonally
    if (std::abs(toCol - fromCol) == 1 && toRow == expectedRow && getPieceAt(toRow, toCol) != -1) {
        return true;
    }
    
    // En passant
    if (std::abs(toCol - fromCol) == 1 && toRow == expectedRow && 
        lastEnPassantRow == toRow && lastEnPassantCol == toCol) {
        return true;
    }
    
    return false;
}

bool MoveValidator::isRookMove(int fromRow, int fromCol, int toRow, int toCol) {
    return (fromRow == toRow) || (fromCol == toCol);
}

bool MoveValidator::isKnightMove(int fromRow, int fromCol, int toRow, int toCol) {
    int rowDiff = std::abs(toRow - fromRow);
    int colDiff = std::abs(toCol - fromCol);
    return (rowDiff == 2 && colDiff == 1) || (rowDiff == 1 && colDiff == 2);
}

bool MoveValidator::isBishopMove(int fromRow, int fromCol, int toRow, int toCol) {
    int rowDiff = std::abs(toRow - fromRow);
    int colDiff = std::abs(toCol - fromCol);
    return rowDiff == colDiff && rowDiff > 0;
}

bool MoveValidator::isQueenMove(int fromRow, int fromCol, int toRow, int toCol) {
    return isRookMove(fromRow, fromCol, toRow, toCol) || isBishopMove(fromRow, fromCol, toRow, toCol);
}

bool MoveValidator::isKingMove(int fromRow, int fromCol, int toRow, int toCol) {
    int rowDiff = std::abs(toRow - fromRow);
    int colDiff = std::abs(toCol - fromCol);
    return rowDiff <= 1 && colDiff <= 1 && (rowDiff > 0 || colDiff > 0);
}

bool MoveValidator::isPathClear(int fromRow, int fromCol, int toRow, int toCol) {
    if (fromRow == toRow) return isHorizontalPathClear(fromRow, fromCol, toRow, toCol);
    if (fromCol == toCol) return isVerticalPathClear(fromRow, fromCol, toRow, toCol);
    return isDiagonalPathClear(fromRow, fromCol, toRow, toCol);
}

bool MoveValidator::isHorizontalPathClear(int fromRow, int fromCol, int toRow, int toCol) {
    int minCol = std::min(fromCol, toCol);
    int maxCol = std::max(fromCol, toCol);
    for (int col = minCol + 1; col < maxCol; col++) {
        if (getPieceAt(fromRow, col) != -1) return false;
    }
    return true;
}

bool MoveValidator::isVerticalPathClear(int fromRow, int fromCol, int toRow, int toCol) {
    int minRow = std::min(fromRow, toRow);
    int maxRow = std::max(fromRow, toRow);
    for (int row = minRow + 1; row < maxRow; row++) {
        if (getPieceAt(row, fromCol) != -1) return false;
    }
    return true;
}

bool MoveValidator::isDiagonalPathClear(int fromRow, int fromCol, int toRow, int toCol) {
    int rowDir = (toRow > fromRow) ? 1 : -1;
    int colDir = (toCol > fromCol) ? 1 : -1;
    
    int row = fromRow + rowDir;
    int col = fromCol + colDir;
    
    while (row != toRow) {
        if (getPieceAt(row, col) != -1) return false;
        row += rowDir;
        col += colDir;
    }
    return true;
}

bool MoveValidator::isSquareAttacked(int row, int col, int byColor) {
    // Check all enemy pieces to see if they can attack this square
    // This version does NOT check if the attacking move would leave the attacking king in check
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            int piece = getPieceAt(r, c);
            if (piece == -1) continue;
            
            int pieceColor = (piece % 2 == 0) ? WHITE : BLACK;
            if (pieceColor != byColor) continue;
            
            // Check if this piece can move to the target square
            // WITHOUT checking if it would leave the king in check (to avoid infinite recursion)
            
            // Check bounds
            if (r < 0 || r > 7 || c < 0 || c > 7 ||
                row < 0 || row > 7 || col < 0 || col > 7) {
                continue;
            }
            
            // Can't move to the same square
            if (r == row && c == col) continue;
            
            int targetPiece = getPieceAt(row, col);
            
            // Can't capture own piece
            if (targetPiece != -1) {
                int targetColor = (targetPiece % 2 == 0) ? WHITE : BLACK;
                if (targetColor == byColor) continue;
            }
            
            // Get the base piece type (0-5)
            int basePiece = piece / 2;
            
            // Validate move based on piece type (WITHOUT king-in-check validation)
            bool canMove = false;
            switch (basePiece) {
                case 0:  // Pawn
                    // Pawns attack diagonally
                    if (byColor == WHITE) {
                        // White pawns move up (decrease row)
                        if (r - row == 1 && abs(c - col) == 1) {
                            canMove = true;
                        }
                    } else {
                        // Black pawns move down (increase row)
                        if (row - r == 1 && abs(c - col) == 1) {
                            canMove = true;
                        }
                    }
                    break;
                case 1:  // Rook
                    if ((r == row || c == col) && isPathClear(r, c, row, col)) {
                        canMove = true;
                    }
                    break;
                case 2:  // Knight
                    if (isKnightMove(r, c, row, col)) {
                        canMove = true;
                    }
                    break;
                case 3:  // Bishop
                    if (isBishopMove(r, c, row, col) && isPathClear(r, c, row, col)) {
                        canMove = true;
                    }
                    break;
                case 4:  // Queen
                    if (isQueenMove(r, c, row, col) && isPathClear(r, c, row, col)) {
                        canMove = true;
                    }
                    break;
                case 5:  // King
                    if (isKingMove(r, c, row, col)) {
                        canMove = true;
                    }
                    break;
            }
            
            if (canMove) {
                return true;
            }
        }
    }
    return false;
}

bool MoveValidator::isKingInCheck(int playerColor) {
    // Find king position
    int kingPiece = (playerColor == WHITE) ? 10 : 11;
    int kingRow = -1, kingCol = -1;
    
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            if (getPieceAt(r, c) == kingPiece) {
                kingRow = r;
                kingCol = c;
                break;
            }
        }
        if (kingRow != -1) break;
    }
    
    if (kingRow == -1) return false;  // King not found (shouldn't happen)
    
    int enemyColor = (playerColor == WHITE) ? BLACK : WHITE;
    return isSquareAttacked(kingRow, kingCol, enemyColor);
}

bool MoveValidator::executeMove(const Move& move, int playerColor) {
    if (!isValidMove(move.fromRow, move.fromCol, move.toRow, move.toCol, playerColor)) {
        return false;
    }
    
    int piece = getPieceAt(move.fromRow, move.fromCol);
    int targetPiece = getPieceAt(move.toRow, move.toCol);
    
    // Check for en passant
    bool isEnPassant = false;
    if ((piece == 0 || piece == 1) && std::abs(move.toCol - move.fromCol) == 1 && targetPiece == -1) {
        isEnPassant = true;
        // Remove the captured pawn
        int capturedRow = move.fromRow;
        engine->clearSquare(capturedRow, move.toCol);
        targetPiece = (piece == 0) ? 1 : 0;  // Opposite color pawn
        std::cout << BitboardEngine::getPieceChar(targetPiece) << BitboardEngine::squareToAlgebraic(capturedRow, move.toCol) 
                  << " captured en passant" << std::endl;
    }
    
    // Execute the move
    engine->movePiece(move.fromRow, move.fromCol, move.toRow, move.toCol);
    
    // Handle pawn promotion
    if ((piece == 0 || piece == 1) && ((playerColor == WHITE && move.toRow == 0) || (playerColor == BLACK && move.toRow == 7))) {
        std::cout << "Pawn promoted at " << BitboardEngine::squareToAlgebraic(move.toRow, move.toCol) << std::endl;
    }
    
    // Check for double pawn push (for en passant next move)
    if ((piece == 0 || piece == 1) && std::abs(move.toRow - move.fromRow) == 2) {
        setLastEnPassantSquare(move.toRow, move.toCol);
    } else {
        clearEnPassantSquare();
    }
    
    // Log check status
    int enemyColor = (playerColor == WHITE) ? BLACK : WHITE;
    if (isKingInCheck(enemyColor)) {
        std::cout << "Check!" << std::endl;
    }
    
    return true;
}

std::vector<Move> MoveValidator::getValidMoves(int row, int col, int playerColor) {
    std::vector<Move> moves;
    
    int piece = getPieceAt(row, col);
    if (piece == -1) return moves;  // No piece at this position
    
    // Check all possible destination squares
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            if (r == row && c == col) continue;  // Skip source square
            
            // Check if this move is valid
            if (isValidMove(row, col, r, c, playerColor)) {
                Move move(row, col, r, c);
                moves.push_back(move);
            }
        }
    }
    
    return moves;
}

bool MoveValidator::isCheckmate(int playerColor) {
    // Checkmate requires: 1) King is in check, 2) No legal moves available
    
    if (!isKingInCheck(playerColor)) {
        return false;  // Not in check, so can't be checkmate
    }
    
    // Try all possible moves for the player - if any move is legal, it's not checkmate
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            int piece = getPieceAt(r, c);
            
            // Skip empty squares and opponent pieces
            if (piece == -1) continue;
            int pieceColor = (piece % 2 == 0) ? WHITE : BLACK;
            if (pieceColor != playerColor) continue;
            
            // Get valid moves for this piece
            std::vector<Move> moves = getValidMoves(r, c, playerColor);
            if (!moves.empty()) {
                return false;  // Found a legal move, not checkmate
            }
        }
    }
    
    // No legal moves and in check = checkmate
    return true;
}
