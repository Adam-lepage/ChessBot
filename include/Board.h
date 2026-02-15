#pragma once

#include <SFML/Graphics.hpp>
#include "BitboardEngine.h"
#include <vector>
#include <tuple>

class Board {
public:
    Board();
    ~Board();
    
    void draw(sf::RenderWindow& window, const sf::Font* font = nullptr);
    void initializePieces();  // Reset bitboard to starting position
    BitboardEngine& getBitboardEngine() { return bitboardEngine; }
    const BitboardEngine& getBitboardEngine() const { return bitboardEngine; }
    
    // Drag support
    void setDraggedPiece(int row, int col, float screenX, float screenY);
    void clearDraggedPiece();
    
    // Move indicators (each entry: row, col, isCapture)
    void drawMoveIndicators(sf::RenderWindow& window, const std::vector<std::tuple<int, int, bool>>& validMoves);
    
    // Selected square highlight
    void drawSelectedSquare(sf::RenderWindow& window, int row, int col);
    
    // Promotion UI
    void drawPromotionUI(sf::RenderWindow& window, int col, int playerColor);
    int getPromotionChoice(float worldX, float worldY, int col, int playerColor) const;

    static const int BOARD_SIZE = 8;
    static const int SQUARE_SIZE = 128;
    static const int BOARD_OFFSET = 60;  // Space for labels

private:
    // Bitboard engine — single source of truth for piece positions
    BitboardEngine bitboardEngine;
    
    // Pre-loaded textures for all 12 piece types (indexed by BitboardEngine piece constants)
    sf::Texture pieceTextures[12];
    sf::Sprite pieceSprites[12];
    bool texturesLoaded;
    
    sf::Color lightSquareColor;
    sf::Color darkSquareColor;
    
    // Drag support
    int draggedPieceType;  // BitboardEngine piece constant of dragged piece
    int draggedRow, draggedCol;
    float draggedScreenX, draggedScreenY;
    bool isDraggingPiece;
    
    void loadTextures();
    void drawSquares(sf::RenderWindow& window);
    void drawPieces(sf::RenderWindow& window);
    void drawBoardLabels(sf::RenderWindow& window, const sf::Font* font);
};
