#pragma once

#include <SFML/Graphics.hpp>
#include "Board.h"
#include "MoveValidator.h"
#include <vector>

class Game {
public:
    Game();
    ~Game();
    
    void run();
    void handleInput();
    void update();
    void render();

private:
    sf::RenderWindow window;
    Board board;
    MoveValidator moveValidator;
    bool isRunning;
    sf::Font font;
    bool isFullscreen;
    
    // Board rendering with fixed aspect ratio
    sf::View boardView;
    static const int BOARD_DISPLAY_SIZE = 1024;  // Fixed board size
    
    // Game state
    int currentPlayer;  // 0 = white, 1 = black
    int selectedRow, selectedCol;  // Selected piece
    bool pieceSelected;
    bool isInCheck;
    bool isCheckmate;
    bool isGameOver;
    
    // Drag state
    bool isDragging;
    sf::Vector2f dragOffset;
    
    // Move indicators
    std::vector<std::pair<int, int>> validMoves;  // Valid moves for selected piece
    
    static const int WHITE = 0;
    static const int BLACK = 1;
    
    // Helper functions
    void updateBoardView();  // Update view before input processing
    void handleBoardClick(sf::Vector2f worldPos);
    void updateDrag(sf::Vector2f worldPos);
    void completeDrag(sf::Vector2f worldPos);
    void calculateValidMoves();  // Calculate valid moves for selected piece
    void checkForCheckmate();  // Check if current player is in checkmate
    void restartGame();  // Restart the game
};
