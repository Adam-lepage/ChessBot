#pragma once

#include <SFML/Graphics.hpp>
#include "Piece.h"
#include "BitboardEngine.h"
#include <array>
#include <memory>

class Board {
public:
    Board();
    ~Board();
    
    void draw(sf::RenderWindow& window, const sf::Font* font = nullptr);
    void initializePieces();
    Piece* getPieceAt(int row, int col);
    BitboardEngine& getBitboardEngine() { return bitboardEngine; }
    
    // Drag support
    void setDraggedPiece(int row, int col, float screenX, float screenY);
    void clearDraggedPiece();
    void movePieceOnBoard(int fromRow, int fromCol, int toRow, int toCol);
    
    // Move indicators
    void drawMoveIndicators(sf::RenderWindow& window, const std::vector<std::pair<int, int>>& validMoves);

private:
    static const int BOARD_SIZE = 8;
    static const int SQUARE_SIZE = 128;
    static const int BOARD_OFFSET = 60;  // Space for labels
    
    // 8x8 board of pieces (nullptr if empty)
    std::array<std::array<std::unique_ptr<Piece>, BOARD_SIZE>, BOARD_SIZE> squares;
    
    // Bitboard engine for tracking pieces
    BitboardEngine bitboardEngine;
    
    sf::Color lightSquareColor;
    sf::Color darkSquareColor;
    
    // Drag support
    int draggedRow, draggedCol;
    float draggedScreenX, draggedScreenY;
    bool isDraggingPiece;
    
    void drawSquares(sf::RenderWindow& window);
    void drawPieces(sf::RenderWindow& window);
    void drawBoardLabels(sf::RenderWindow& window, const sf::Font* font);
};
