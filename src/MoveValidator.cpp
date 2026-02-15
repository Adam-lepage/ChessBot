#include "MoveValidator.h"
#include <iostream>
#include <cmath>
#include <cstdint>

MoveValidator::MoveValidator(BitboardEngine* engine) 
    : engine(engine), lastEnPassantRow(-1), lastEnPassantCol(-1),
      whiteKingsideCastle(true), whiteQueensideCastle(true),
      blackKingsideCastle(true), blackQueensideCastle(true) {
}

MoveValidator::~MoveValidator() = default;

Bitboard* MoveValidator::getBitboardForPiece(int piece) {
    switch (piece) {
        case BitboardEngine::WHITE_PAWN:   return &engine->pawns[0];
        case BitboardEngine::BLACK_PAWN:   return &engine->pawns[1];
        case BitboardEngine::WHITE_ROOK:   return &engine->rooks[0];
        case BitboardEngine::BLACK_ROOK:   return &engine->rooks[1];
        case BitboardEngine::WHITE_KNIGHT: return &engine->knights[0];
        case BitboardEngine::BLACK_KNIGHT: return &engine->knights[1];
        case BitboardEngine::WHITE_BISHOP: return &engine->bishops[0];
        case BitboardEngine::BLACK_BISHOP: return &engine->bishops[1];
        case BitboardEngine::WHITE_QUEEN:  return &engine->queens[0];
        case BitboardEngine::BLACK_QUEEN:  return &engine->queens[1];
        case BitboardEngine::WHITE_KING:   return &engine->kings[0];
        case BitboardEngine::BLACK_KING:   return &engine->kings[1];
        default: return nullptr;
    }
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
            if (!isKingMove(fromRow, fromCol, toRow, toCol) &&
                !isCastlingMove(fromRow, fromCol, toRow, toCol, playerColor)) return false;
            break;
        default:
            return false;
    }
    
    // Simulate the move and check if the king would be in check
    int fromIndex = fromRow * 8 + fromCol;
    int toIndex = toRow * 8 + toCol;
    
    // Castling is already fully validated by isCastlingMove - skip generic simulation
    if (basePiece == 5 && std::abs(toCol - fromCol) == 2) {
        return true;  // isCastlingMove already checked all squares for attacks
    }
    
    Bitboard* sourceBitboard = getBitboardForPiece(piece);
    if (!sourceBitboard) return false;
    
    Bitboard* targetBitboard = (targetPiece != -1) ? getBitboardForPiece(targetPiece) : nullptr;
    
    // Handle en passant capture in simulation (pawn diagonal to empty square)
    Bitboard* enPassantBitboard = nullptr;
    int enPassantIndex = -1;
    if (piece / 2 == 0 && std::abs(toCol - fromCol) == 1 && targetPiece == -1) {
        int capturedPawn = (playerColor == WHITE) ? BitboardEngine::BLACK_PAWN : BitboardEngine::WHITE_PAWN;
        enPassantBitboard = getBitboardForPiece(capturedPawn);
        enPassantIndex = fromRow * 8 + toCol;
    }
    
    // Save old state
    Bitboard oldSourceBB = *sourceBitboard;
    Bitboard oldTargetBB = targetBitboard ? *targetBitboard : 0;
    Bitboard oldEnPassantBB = enPassantBitboard ? *enPassantBitboard : 0;
    Bitboard oldAllWhite = engine->allWhitePieces;
    Bitboard oldAllBlack = engine->allBlackPieces;
    Bitboard oldAllPieces = engine->allPieces;
    
    // Simulate the move
    *sourceBitboard &= ~(1ULL << fromIndex);
    *sourceBitboard |= (1ULL << toIndex);
    if (targetBitboard) *targetBitboard &= ~(1ULL << toIndex);
    if (enPassantBitboard) *enPassantBitboard &= ~(1ULL << enPassantIndex);
    engine->updateCombinedBitboards();
    
    bool kingInCheck = isKingInCheck(playerColor);
    
    // Restore old state
    *sourceBitboard = oldSourceBB;
    if (targetBitboard) *targetBitboard = oldTargetBB;
    if (enPassantBitboard) *enPassantBitboard = oldEnPassantBB;
    engine->allWhitePieces = oldAllWhite;
    engine->allBlackPieces = oldAllBlack;
    engine->allPieces = oldAllPieces;
    
    return !kingInCheck;
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
            
            // Can't attack from the same square
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
    // Use bitboard to find king position directly
    int colorIdx = (playerColor == WHITE) ? 0 : 1;
    Bitboard kingBB = engine->kings[colorIdx];
    if (kingBB == 0) return false;  // King not found (shouldn't happen)
    
    // Find the set bit index
    int index = __builtin_ctzll(kingBB);
    int kingRow, kingCol;
    BitboardEngine::indexToSquare(index, kingRow, kingCol);
    
    int enemyColor = (playerColor == WHITE) ? BLACK : WHITE;
    return isSquareAttacked(kingRow, kingCol, enemyColor);
}

bool MoveValidator::executeMove(Move& move, int playerColor) {
    if (!isValidMove(move.fromRow, move.fromCol, move.toRow, move.toCol, playerColor)) {
        return false;
    }
    
    int piece = getPieceAt(move.fromRow, move.fromCol);
    int targetPiece = getPieceAt(move.toRow, move.toCol);
    move.capturedPiece = targetPiece;
    
    // Check for castling
    if (piece / 2 == 5 && std::abs(move.toCol - move.fromCol) == 2) {
        move.isCastling = true;
        engine->movePiece(move.fromRow, move.fromCol, move.toRow, move.toCol);
        
        // Move the rook
        if (move.toCol == 6) {  // Kingside
            engine->movePiece(move.fromRow, 7, move.fromRow, 5);
        } else {  // Queenside (toCol == 2)
            engine->movePiece(move.fromRow, 0, move.fromRow, 3);
        }
        
        updateCastlingRights(piece, move.fromRow, move.fromCol);
        clearEnPassantSquare();
        
        int enemyColor = (playerColor == WHITE) ? BLACK : WHITE;
        if (isKingInCheck(enemyColor)) {
            std::cout << "Check!" << std::endl;
        }
        return true;
    }
    
    // Check for en passant
    if (piece / 2 == 0 && std::abs(move.toCol - move.fromCol) == 1 && targetPiece == -1) {
        move.isEnPassant = true;
        int capturedRow = move.fromRow;
        engine->clearSquare(capturedRow, move.toCol);
        move.capturedPiece = (piece == BitboardEngine::WHITE_PAWN) ? BitboardEngine::BLACK_PAWN : BitboardEngine::WHITE_PAWN;
    }
    
    // Execute the move on bitboard
    engine->movePiece(move.fromRow, move.fromCol, move.toRow, move.toCol);
    
    // Update castling rights based on what moved/was captured
    updateCastlingRights(piece, move.fromRow, move.fromCol);
    if (targetPiece != -1) {
        updateCastlingRights(targetPiece, move.toRow, move.toCol);
    }
    
    // Handle pawn promotion — use caller's choice if set, otherwise default to queen
    if (piece / 2 == 0 && ((playerColor == WHITE && move.toRow == 0) || (playerColor == BLACK && move.toRow == 7))) {
        move.isPawnPromotion = true;
        int promotedPiece;
        if (move.promotedTo != -1) {
            promotedPiece = move.promotedTo;  // Caller specified promotion choice
        } else {
            promotedPiece = (playerColor == WHITE) ? BitboardEngine::WHITE_QUEEN : BitboardEngine::BLACK_QUEEN;
        }
        engine->clearSquare(move.toRow, move.toCol);
        engine->setPieceAt(move.toRow, move.toCol, promotedPiece);
        move.promotedTo = promotedPiece;
    }
    
    // Track en passant square (store the passed-through square, not the landing square)
    if (piece / 2 == 0 && std::abs(move.toRow - move.fromRow) == 2) {
        setLastEnPassantSquare((move.fromRow + move.toRow) / 2, move.toCol);
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

bool MoveValidator::hasAnyLegalMoves(int playerColor) {
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            int piece = getPieceAt(r, c);
            if (piece == -1) continue;
            int pieceColor = (piece % 2 == 0) ? WHITE : BLACK;
            if (pieceColor != playerColor) continue;
            
            std::vector<Move> moves = getValidMoves(r, c, playerColor);
            if (!moves.empty()) {
                return true;
            }
        }
    }
    return false;
}

bool MoveValidator::isCheckmate(int playerColor) {
    return isKingInCheck(playerColor) && !hasAnyLegalMoves(playerColor);
}

bool MoveValidator::isStalemate(int playerColor) {
    return !isKingInCheck(playerColor) && !hasAnyLegalMoves(playerColor);
}

bool MoveValidator::isCastlingMove(int fromRow, int fromCol, int toRow, int toCol, int playerColor) {
    // King must move exactly 2 squares horizontally, same row
    if (fromRow != toRow) return false;
    if (std::abs(toCol - fromCol) != 2) return false;
    
    int backRank = (playerColor == WHITE) ? 7 : 0;
    if (fromRow != backRank) return false;
    if (fromCol != 4) return false;  // King must be on e-file
    
    bool kingside = (toCol == 6);
    bool queenside = (toCol == 2);
    if (!kingside && !queenside) return false;
    
    // Check castling rights
    if (playerColor == WHITE) {
        if (kingside && !whiteKingsideCastle) return false;
        if (queenside && !whiteQueensideCastle) return false;
    } else {
        if (kingside && !blackKingsideCastle) return false;
        if (queenside && !blackQueensideCastle) return false;
    }
    
    // Check that rook is in place
    int rookCol = kingside ? 7 : 0;
    int expectedRook = (playerColor == WHITE) ? BitboardEngine::WHITE_ROOK : BitboardEngine::BLACK_ROOK;
    if (getPieceAt(backRank, rookCol) != expectedRook) return false;
    
    // Path between king and rook must be clear
    int minCol = std::min(fromCol, rookCol) + 1;
    int maxCol = std::max(fromCol, rookCol);
    for (int c = minCol; c < maxCol; c++) {
        if (getPieceAt(backRank, c) != -1) return false;
    }
    
    // King must not be in check, and must not pass through or land on attacked squares
    int enemyColor = (playerColor == WHITE) ? BLACK : WHITE;
    if (isSquareAttacked(backRank, 4, enemyColor)) return false;  // Can't castle out of check
    
    int step = kingside ? 1 : -1;
    for (int c = fromCol + step; c != toCol + step; c += step) {
        if (isSquareAttacked(backRank, c, enemyColor)) return false;
    }
    
    return true;
}

void MoveValidator::updateCastlingRights(int piece, int fromRow, int fromCol) {
    // If king moves, lose both castling rights
    if (piece == BitboardEngine::WHITE_KING) {
        whiteKingsideCastle = false;
        whiteQueensideCastle = false;
    } else if (piece == BitboardEngine::BLACK_KING) {
        blackKingsideCastle = false;
        blackQueensideCastle = false;
    }
    // If rook moves or is captured, lose that side's castling right
    else if (piece == BitboardEngine::WHITE_ROOK) {
        if (fromRow == 7 && fromCol == 7) whiteKingsideCastle = false;
        if (fromRow == 7 && fromCol == 0) whiteQueensideCastle = false;
    } else if (piece == BitboardEngine::BLACK_ROOK) {
        if (fromRow == 0 && fromCol == 7) blackKingsideCastle = false;
        if (fromRow == 0 && fromCol == 0) blackQueensideCastle = false;
    }
}

void MoveValidator::resetCastlingRights() {
    whiteKingsideCastle = true;
    whiteQueensideCastle = true;
    blackKingsideCastle = true;
    blackQueensideCastle = true;
}

bool MoveValidator::canCastleKingside(int playerColor) const {
    return (playerColor == WHITE) ? whiteKingsideCastle : blackKingsideCastle;
}

bool MoveValidator::canCastleQueenside(int playerColor) const {
    return (playerColor == WHITE) ? whiteQueensideCastle : blackQueensideCastle;
}
