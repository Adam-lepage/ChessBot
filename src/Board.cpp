#include "Board.h"
#include <iostream>

extern bool g_debugOutput;

/* This file handles the board rendering and piece management */

// Texture file paths indexed by BitboardEngine piece constants
static const char* TEXTURE_FILES[12] = {
    "assets/pieces/pawn-w.png",    // WHITE_PAWN   = 0
    "assets/pieces/pawn-b.png",    // BLACK_PAWN   = 1
    "assets/pieces/rook-w.png",    // WHITE_ROOK   = 2
    "assets/pieces/rook-b.png",    // BLACK_ROOK   = 3
    "assets/pieces/knight-w.png",  // WHITE_KNIGHT = 4
    "assets/pieces/knight-b.png",  // BLACK_KNIGHT = 5
    "assets/pieces/bishop-w.png",  // WHITE_BISHOP = 6
    "assets/pieces/bishop-b.png",  // BLACK_BISHOP = 7
    "assets/pieces/queen-w.png",   // WHITE_QUEEN  = 8
    "assets/pieces/queen-b.png",   // BLACK_QUEEN  = 9
    "assets/pieces/king-w.png",    // WHITE_KING   = 10
    "assets/pieces/king-b.png",    // BLACK_KING   = 11
};

// Initialize the board to the standard starting position using bitboards
Board::Board() 
    : texturesLoaded(false),
      lightSquareColor(240, 217, 181),  // Light tan
      darkSquareColor(181, 136, 99),    // Brown
      draggedPieceType(-1),
      draggedRow(-1),
      draggedCol(-1),
      draggedScreenX(0),
      draggedScreenY(0),
      isDraggingPiece(false) {
    loadTextures();
    initializePieces();
}

Board::~Board() = default;

// load piece textures and prepare sprites
void Board::loadTextures() {
    texturesLoaded = true;
    for (int i = 0; i < 12; i++) {
        if (pieceTextures[i].loadFromFile(TEXTURE_FILES[i])) {
            pieceTextures[i].setSmooth(true);
            pieceSprites[i].setTexture(pieceTextures[i]);
            
            // Scale to fit within square
            float maxDim = SQUARE_SIZE * 0.8f;
            float tw = static_cast<float>(pieceTextures[i].getSize().x);
            float th = static_cast<float>(pieceTextures[i].getSize().y);
            float scale = std::min(maxDim / tw, maxDim / th);
            pieceSprites[i].setScale(scale, scale);
            
            // Set origin to center of sprite
            sf::FloatRect bounds = pieceSprites[i].getLocalBounds();
            pieceSprites[i].setOrigin(bounds.width / 2.0f, bounds.height / 2.0f);
        } else {
            std::cerr << "Failed to load texture: " << TEXTURE_FILES[i] << std::endl;
            texturesLoaded = false;
        }
    }
}

void Board::initializePieces() {
    bitboardEngine.initializeStartingPosition();
}

// Draw the board, pieces, and labels
void Board::draw(sf::RenderWindow& window, const sf::Font* font) {
    drawSquares(window);
    drawPieces(window);
    drawBoardLabels(window, font);
}

// Draw the squares of the chessboard with alternating colors
void Board::drawSquares(sf::RenderWindow& window) {
    for (int row = 0; row < BOARD_SIZE; row++) {
        for (int col = 0; col < BOARD_SIZE; col++) {
            sf::RectangleShape square(sf::Vector2f(SQUARE_SIZE, SQUARE_SIZE));
            square.setPosition(BOARD_OFFSET + col * SQUARE_SIZE, BOARD_OFFSET + row * SQUARE_SIZE);
            
            if ((row + col) % 2 == 0) {
                square.setFillColor(lightSquareColor);
            } else {
                square.setFillColor(darkSquareColor);
            }
            
            window.draw(square);
        }
    }
}

// Draw pieces based on the current state of the bitboards, skipping the dragged piece which is drawn separately
void Board::drawPieces(sf::RenderWindow& window) {
    if (!texturesLoaded) return;
    
    // Walk each of the 12 bitboards and draw pieces directly
    for (int pieceType = 0; pieceType < 12; pieceType++) {
        // Get the bitboard for this piece type
        Bitboard bb;
        int colorIdx = pieceType % 2;  // 0=white, 1=black
        switch (pieceType / 2) {
            case 0: bb = bitboardEngine.pawns[colorIdx]; break;
            case 1: bb = bitboardEngine.rooks[colorIdx]; break;
            case 2: bb = bitboardEngine.knights[colorIdx]; break;
            case 3: bb = bitboardEngine.bishops[colorIdx]; break;
            case 4: bb = bitboardEngine.queens[colorIdx]; break;
            case 5: bb = bitboardEngine.kings[colorIdx]; break;
            default: bb = 0; break;
        }
        
        while (bb) {
            int index = __builtin_ctzll(bb);
            int row = index / 8;
            int col = index % 8;
            
            // Skip the dragged piece
            if (isDraggingPiece && row == draggedRow && col == draggedCol) {
                bb &= bb - 1;  // Clear lowest set bit
                continue;
            }
            
            float centerX = BOARD_OFFSET + col * SQUARE_SIZE + SQUARE_SIZE / 2.0f;
            float centerY = BOARD_OFFSET + row * SQUARE_SIZE + SQUARE_SIZE / 2.0f;
            pieceSprites[pieceType].setPosition(centerX, centerY);
            window.draw(pieceSprites[pieceType]);
            
            bb &= bb - 1;  // Clear lowest set bit
        }
    }
    
    // Draw dragged piece on top at cursor position
    if (isDraggingPiece && draggedPieceType >= 0 && draggedPieceType < 12) {
        pieceSprites[draggedPieceType].setPosition(draggedScreenX, draggedScreenY);
        window.draw(pieceSprites[draggedPieceType]);
    }
}

// Draw labels around the board (a-h, 1-8)
void Board::drawBoardLabels(sf::RenderWindow& window, const sf::Font* font) {
    if (!font) return;
    
    sf::Text text;
    text.setFont(*font);
    text.setCharacterSize(18);
    text.setFillColor(sf::Color::Black);
    
    for (int col = 0; col < BOARD_SIZE; col++) {
        text.setString(std::string(1, 'a' + col));
        text.setPosition(BOARD_OFFSET + col * SQUARE_SIZE + SQUARE_SIZE / 2 - 6, 
                        BOARD_OFFSET + BOARD_SIZE * SQUARE_SIZE + 10);
        window.draw(text);
    }
    
    for (int row = 0; row < BOARD_SIZE; row++) {
        text.setString(std::string(1, '8' - row));
        text.setPosition(BOARD_OFFSET - 30, 
                        BOARD_OFFSET + row * SQUARE_SIZE + SQUARE_SIZE / 2 - 10);
        window.draw(text);
    }
}

// Store the piece being dragged along with its original position and current screen coordinates
void Board::setDraggedPiece(int row, int col, float screenX, float screenY) {
    draggedRow = row;
    draggedCol = col;
    draggedPieceType = bitboardEngine.getPieceAt(row, col);
    draggedScreenX = screenX;
    draggedScreenY = screenY;
    isDraggingPiece = true;
}

// Clear the dragged piece state when dropping or cancelling a drag
void Board::clearDraggedPiece() {
    isDraggingPiece = false;
    draggedPieceType = -1;
    draggedRow = -1;
    draggedCol = -1;
}

// Draw move indicators for valid moves of the selected piece
void Board::drawMoveIndicators(sf::RenderWindow& window, const std::vector<std::tuple<int, int, bool>>& validMoves) {
    for (const auto& [row, col, isCapture] : validMoves) {
        float centerX = BOARD_OFFSET + col * SQUARE_SIZE + SQUARE_SIZE / 2.0f;  
        float centerY = BOARD_OFFSET + row * SQUARE_SIZE + SQUARE_SIZE / 2.0f;
        
        if (isCapture) {
            float thickness = 7.0f;
            float innerRadius = SQUARE_SIZE / 2.0f - thickness;
            sf::CircleShape ring(innerRadius);
            ring.setFillColor(sf::Color::Transparent);
            ring.setOutlineThickness(thickness);
            ring.setOutlineColor(sf::Color(0, 0, 0, 60));
            ring.setPosition(centerX - innerRadius, centerY - innerRadius);
            window.draw(ring);
        } else {
            float dotRadius = SQUARE_SIZE * 0.13f;
            sf::CircleShape dot(dotRadius);
            dot.setFillColor(sf::Color(0, 0, 0, 60));
            dot.setPosition(centerX - dotRadius, centerY - dotRadius);
            window.draw(dot);
        }
    }
}

// Highlight the selected square with a semi-transparent overlay
void Board::drawSelectedSquare(sf::RenderWindow& window, int row, int col) {
    if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE) return;
    
    sf::RectangleShape overlay(sf::Vector2f(SQUARE_SIZE, SQUARE_SIZE));
    overlay.setPosition(BOARD_OFFSET + col * SQUARE_SIZE, BOARD_OFFSET + row * SQUARE_SIZE);
    overlay.setFillColor(sf::Color(0, 0, 0, 60));
    window.draw(overlay);
}

// Draw the mini promotion UI with piece options when a pawn is promoting to choose what to promote to
void Board::drawPromotionUI(sf::RenderWindow& window, int col, int playerColor) {
    if (!texturesLoaded) return;
    
    int promotionPieces[4];
    if (playerColor == 0) {  // White
        promotionPieces[0] = BitboardEngine::WHITE_QUEEN;
        promotionPieces[1] = BitboardEngine::WHITE_ROOK;
        promotionPieces[2] = BitboardEngine::WHITE_BISHOP;
        promotionPieces[3] = BitboardEngine::WHITE_KNIGHT;
    } else {  // Black
        promotionPieces[0] = BitboardEngine::BLACK_QUEEN;
        promotionPieces[1] = BitboardEngine::BLACK_ROOK;
        promotionPieces[2] = BitboardEngine::BLACK_BISHOP;
        promotionPieces[3] = BitboardEngine::BLACK_KNIGHT;
    }
    
    // Semi-transparent overlay over the entire board to focus attention on promotion choices
    sf::RectangleShape boardOverlay(sf::Vector2f(BOARD_SIZE * SQUARE_SIZE, BOARD_SIZE * SQUARE_SIZE));
    boardOverlay.setPosition(BOARD_OFFSET, BOARD_OFFSET);
    boardOverlay.setFillColor(sf::Color(0, 0, 0, 120));
    window.draw(boardOverlay);
    
    // Draw 4 squares with piece options
    // White promotes at row 0, so show choices starting from row 0 going down
    // Black promotes at row 7, so show choices starting from row 4 going down
    int startRow = (playerColor == 0) ? 0 : 4;
    
    for (int i = 0; i < 4; i++) {
        int row = startRow + i;
        float x = BOARD_OFFSET + col * SQUARE_SIZE;
        float y = BOARD_OFFSET + row * SQUARE_SIZE;
        
        // Draw background square
        sf::RectangleShape square(sf::Vector2f(SQUARE_SIZE, SQUARE_SIZE));
        square.setPosition(x, y);
        square.setFillColor(sf::Color(240, 240, 240));
        square.setOutlineThickness(2.0f);
        square.setOutlineColor(sf::Color(60, 60, 60));
        window.draw(square);
        
        // Draw the piece
        int pt = promotionPieces[i];
        float centerX = x + SQUARE_SIZE / 2.0f;
        float centerY = y + SQUARE_SIZE / 2.0f;
        pieceSprites[pt].setPosition(centerX, centerY);
        window.draw(pieceSprites[pt]);
    }
}

// Determine which piece the player clicked on in the promotion UI 
int Board::getPromotionChoice(float worldX, float worldY, int col, int playerColor) const {
    float colLeft = BOARD_OFFSET + col * SQUARE_SIZE;
    float colRight = colLeft + SQUARE_SIZE;
    
    if (worldX < colLeft || worldX > colRight) return -1;
    
    int startRow = (playerColor == 0) ? 0 : 4;
    
    int promotionPieces[4];
    if (playerColor == 0) {
        promotionPieces[0] = BitboardEngine::WHITE_QUEEN;
        promotionPieces[1] = BitboardEngine::WHITE_ROOK;
        promotionPieces[2] = BitboardEngine::WHITE_BISHOP;
        promotionPieces[3] = BitboardEngine::WHITE_KNIGHT;
    } else {
        promotionPieces[0] = BitboardEngine::BLACK_QUEEN;
        promotionPieces[1] = BitboardEngine::BLACK_ROOK;
        promotionPieces[2] = BitboardEngine::BLACK_BISHOP;
        promotionPieces[3] = BitboardEngine::BLACK_KNIGHT;
    }
    
    for (int i = 0; i < 4; i++) {
        int row = startRow + i;
        float rowTop = BOARD_OFFSET + row * SQUARE_SIZE;
        float rowBottom = rowTop + SQUARE_SIZE;
        
        if (worldY >= rowTop && worldY <= rowBottom) {
            return promotionPieces[i];
        }
    }
    
    return -1;
}
