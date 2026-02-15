#include "BitboardEngine.h"
#include <iostream>
#include <iomanip>

extern bool g_debugOutput;

/* This bitboard is a 64-bit representation of the chessboard, where each bit corresponds to a square. 
   This means each piece type for each color is represented by a separate 64-bit integer, allowing for efficient bitwise operations. */

BitboardEngine::BitboardEngine() {
    initializeStartingPosition();
}

BitboardEngine::~BitboardEngine() = default;

void BitboardEngine::initializeStartingPosition() {
    // Clear all bitboards
    for (int i = 0; i < 2; i++) {
        pawns[i] = 0;
        rooks[i] = 0;
        knights[i] = 0;
        bishops[i] = 0;
        queens[i] = 0;
        kings[i] = 0;
    }
    
    // Set white and black pawns on their respective rows
    for (int col = 0; col < 8; col++) {
        pawns[0] |= (1ULL << squareToIndex(6, col));
        pawns[1] |= (1ULL << squareToIndex(1, col));
    }
    
    // White back rank (row 7)
    rooks[0] |= (1ULL << squareToIndex(7, 0)) | (1ULL << squareToIndex(7, 7));
    knights[0] |= (1ULL << squareToIndex(7, 1)) | (1ULL << squareToIndex(7, 6));
    bishops[0] |= (1ULL << squareToIndex(7, 2)) | (1ULL << squareToIndex(7, 5));
    queens[0] |= (1ULL << squareToIndex(7, 3));
    kings[0] |= (1ULL << squareToIndex(7, 4));
    
    // Black back rank (row 0)
    rooks[1] |= (1ULL << squareToIndex(0, 0)) | (1ULL << squareToIndex(0, 7));
    knights[1] |= (1ULL << squareToIndex(0, 1)) | (1ULL << squareToIndex(0, 6));
    bishops[1] |= (1ULL << squareToIndex(0, 2)) | (1ULL << squareToIndex(0, 5));
    queens[1] |= (1ULL << squareToIndex(0, 3));
    kings[1] |= (1ULL << squareToIndex(0, 4));
    
    // Update combined bitboards (these are useful for clear path checks and move generation)
    allWhitePieces = pawns[0] | rooks[0] | knights[0] | bishops[0] | queens[0] | kings[0];
    allBlackPieces = pawns[1] | rooks[1] | knights[1] | bishops[1] | queens[1] | kings[1];
    allPieces = allWhitePieces | allBlackPieces;
}

// Convert (row, col) to a bitboard index (0-63)
int BitboardEngine::squareToIndex(int row, int col) {
    return row * 8 + col;
}

// Convert a bitboard index back to (row, col)
void BitboardEngine::indexToSquare(int index, int& row, int& col) {
    row = index / 8;
    col = index % 8;
}

// Convert (row, col) to algebraic notation (e.g., "a1", "h8")
std::string BitboardEngine::squareToAlgebraic(int row, int col) {
    char file = 'a' + col;
    char rank = '8' - row;  // Row 0 is rank 8, row 7 is rank 1
    return std::string(1, file) + rank;
}

// Maps piece constants to characters for logging and display purposes
char BitboardEngine::getPieceChar(int piece) {
    switch (piece) {
        case WHITE_PAWN:
        case BLACK_PAWN:
            return 'P';
        case WHITE_ROOK:
        case BLACK_ROOK:
            return 'R';
        case WHITE_KNIGHT:
        case BLACK_KNIGHT:
            return 'N';
        case WHITE_BISHOP:
        case BLACK_BISHOP:
            return 'B';
        case WHITE_QUEEN:
        case BLACK_QUEEN:
            return 'Q';
        case WHITE_KING:
        case BLACK_KING:
            return 'K';
        default:
            return ' ';
    }
}

// Get the piece type at a specific square by masking the corresponding bit in each piece's bitboard
int BitboardEngine::getPieceAt(int row, int col) const {
    int index = squareToIndex(row, col);
    uint64_t mask = 1ULL << index;
    
    if (pawns[0] & mask) return WHITE_PAWN;
    if (pawns[1] & mask) return BLACK_PAWN;
    if (rooks[0] & mask) return WHITE_ROOK;
    if (rooks[1] & mask) return BLACK_ROOK;
    if (knights[0] & mask) return WHITE_KNIGHT;
    if (knights[1] & mask) return BLACK_KNIGHT;
    if (bishops[0] & mask) return WHITE_BISHOP;
    if (bishops[1] & mask) return BLACK_BISHOP;
    if (queens[0] & mask) return WHITE_QUEEN;
    if (queens[1] & mask) return BLACK_QUEEN;
    if (kings[0] & mask) return WHITE_KING;
    if (kings[1] & mask) return BLACK_KING;
    
    return EMPTY;
}

// Set a piece at a specific square by updating the corresponding bit in the appropriate piece's bitboard
void BitboardEngine::setPieceAt(int row, int col, int piece) {
    int index = squareToIndex(row, col);
    uint64_t mask = 1ULL << index;
    
    // Clear the square first
    clearSquare(row, col);
    
    // Set the piece
    switch (piece) {
        case WHITE_PAWN:
            pawns[0] |= mask;
            break;
        case BLACK_PAWN:
            pawns[1] |= mask;
            break;
        case WHITE_ROOK:
            rooks[0] |= mask;
            break;
        case BLACK_ROOK:
            rooks[1] |= mask;
            break;
        case WHITE_KNIGHT:
            knights[0] |= mask;
            break;
        case BLACK_KNIGHT:
            knights[1] |= mask;
            break;
        case WHITE_BISHOP:
            bishops[0] |= mask;
            break;
        case BLACK_BISHOP:
            bishops[1] |= mask;
            break;
        case WHITE_QUEEN:
            queens[0] |= mask;
            break;
        case BLACK_QUEEN:
            queens[1] |= mask;
            break;
        case WHITE_KING:
            kings[0] |= mask;
            break;
        case BLACK_KING:
            kings[1] |= mask;
            break;
    }
    
    // Update combined bitboards
    allWhitePieces = pawns[0] | rooks[0] | knights[0] | bishops[0] | queens[0] | kings[0];
    allBlackPieces = pawns[1] | rooks[1] | knights[1] | bishops[1] | queens[1] | kings[1];
    allPieces = allWhitePieces | allBlackPieces;
}

// Clear a square by resetting the corresponding bit in all piece bitboards
void BitboardEngine::clearSquare(int row, int col) {
    int index = squareToIndex(row, col);
    uint64_t mask = ~(1ULL << index);
    
    pawns[0] &= mask;
    pawns[1] &= mask;
    rooks[0] &= mask;
    rooks[1] &= mask;
    knights[0] &= mask;
    knights[1] &= mask;
    bishops[0] &= mask;
    bishops[1] &= mask;
    queens[0] &= mask;
    queens[1] &= mask;
    kings[0] &= mask;
    kings[1] &= mask;
    
    // Update combined bitboards
    allWhitePieces = pawns[0] | rooks[0] | knights[0] | bishops[0] | queens[0] | kings[0];
    allBlackPieces = pawns[1] | rooks[1] | knights[1] | bishops[1] | queens[1] | kings[1];
    allPieces = allWhitePieces | allBlackPieces;
}

// Move a piece from one square to another by clearing the source and setting the destination
void BitboardEngine::movePiece(int fromRow, int fromCol, int toRow, int toCol) {
    int piece = getPieceAt(fromRow, fromCol);
    
    if (piece != EMPTY) {
        // Clear source and destination without updating combined BBs each time
        int fromIndex = squareToIndex(fromRow, fromCol);
        int toIndex = squareToIndex(toRow, toCol);
        uint64_t fromMask = ~(1ULL << fromIndex);
        uint64_t toMask = ~(1ULL << toIndex);
        
        // Clear both squares from all bitboards
        for (int i = 0; i < 2; i++) {
            pawns[i] &= fromMask & toMask;
            rooks[i] &= fromMask & toMask;
            knights[i] &= fromMask & toMask;
            bishops[i] &= fromMask & toMask;
            queens[i] &= fromMask & toMask;
            kings[i] &= fromMask & toMask;
        }
        
        // Place piece at destination
        Bitboard* bb = nullptr;
        int colorIdx = (piece % 2 == 0) ? 0 : 1;
        switch (piece / 2) {
            case 0: bb = &pawns[colorIdx]; break;
            case 1: bb = &rooks[colorIdx]; break;
            case 2: bb = &knights[colorIdx]; break;
            case 3: bb = &bishops[colorIdx]; break;
            case 4: bb = &queens[colorIdx]; break;
            case 5: bb = &kings[colorIdx]; break;
        }
        if (bb) *bb |= (1ULL << toIndex);
        
        // Single combined bitboard update
        updateCombinedBitboards();
        
        // Debug output
        if (g_debugOutput) {
            std::string fromSquare = squareToAlgebraic(fromRow, fromCol);
            std::string toSquare = squareToAlgebraic(toRow, toCol);
            std::cout << "[DEBUG] " << getPieceChar(piece) << fromSquare << "-" << toSquare << std::endl;
        }
    }
}

// Print the current board state to the console in a human-readable format
void BitboardEngine::printBoard() const {
    std::cout << "\n  a b c d e f g h\n";
    
    for (int row = 0; row < 8; row++) {
        std::cout << (8 - row) << " ";
        
        for (int col = 0; col < 8; col++) {
            int piece = getPieceAt(row, col);
            char pieceChar = ' ';
            
            if (piece == WHITE_PAWN) pieceChar = 'P';
            else if (piece == BLACK_PAWN) pieceChar = 'p';
            else if (piece == WHITE_ROOK) pieceChar = 'R';
            else if (piece == BLACK_ROOK) pieceChar = 'r';
            else if (piece == WHITE_KNIGHT) pieceChar = 'N';
            else if (piece == BLACK_KNIGHT) pieceChar = 'n';
            else if (piece == WHITE_BISHOP) pieceChar = 'B';
            else if (piece == BLACK_BISHOP) pieceChar = 'b';
            else if (piece == WHITE_QUEEN) pieceChar = 'Q';
            else if (piece == BLACK_QUEEN) pieceChar = 'q';
            else if (piece == WHITE_KING) pieceChar = 'K';
            else if (piece == BLACK_KING) pieceChar = 'k';
            
            std::cout << pieceChar << " ";
        }
        
        std::cout << (8 - row) << "\n";
    }
    
    std::cout << "  a b c d e f g h\n\n";
}

void BitboardEngine::updateCombinedBitboards() {
    allWhitePieces = pawns[0] | rooks[0] | knights[0] | bishops[0] | queens[0] | kings[0];
    allBlackPieces = pawns[1] | rooks[1] | knights[1] | bishops[1] | queens[1] | kings[1];
    allPieces = allWhitePieces | allBlackPieces;
}
