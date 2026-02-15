#pragma once

#include <SFML/Graphics.hpp>
#include "Board.h"
#include "MoveValidator.h"
#include "ChessBot.h"
#include "GameConfig.h"
#include <vector>

// Global debug flag - controlled by --debug command line argument
extern bool g_debugOutput;

class Game {
public:
    Game(const GameConfig& config);
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
    bool isStalemate;
    bool isDrawByMoveLimit;
    bool isDrawByMaterial;
    bool isGameOver;
    
    // Drag state
    bool isDragging;
    
    // Promotion state
    bool waitingForPromotion;
    Move pendingPromotionMove;
    int promotionCol;  // Column where pawn is promoting
    
    // Move indicators (row, col, isCapture)
    std::vector<std::tuple<int, int, bool>> validMoves;  // Valid moves for selected piece
    
    // Bot players (nullptr = human)
    ChessBot* whiteBot;
    ChessBot* blackBot;

    int halfmoveClock;  // Moves since last capture or pawn move (75-move rule)
    
    // Mode / config
    bool headless;  // true = no GUI (console only)
    GameConfig config;
    
    static const int WHITE = 0;
    static const int BLACK = 1;
    
    // Helper functions
    void updateBoardView();  // Update view before input processing
    void handleBoardClick(sf::Vector2f worldPos);
    void updateDrag(sf::Vector2f worldPos);
    void completeDrag(sf::Vector2f worldPos);
    void executePlayerMove(int targetRow, int targetCol);  // Execute a validated player move
    void completePromotion(int promotedPiece);  // Complete a pawn promotion after user choice
    void handlePromotionClick(sf::Vector2f worldPos);  // Handle click during promotion UI
    void calculateValidMoves();  // Calculate valid moves for selected piece
    void checkForCheckmate();  // Check if current player is in checkmate
    void checkForDrawConditions();  // Check for draw conditions (material/move limit)
    bool onlyKingsLeft() const;  // True when only two kings remain
    void restartGame();  // Restart the game
    bool isBotTurn() const;  // Check if current player is a bot
    void processBotMove();  // Execute bot's chosen move
    void init();  // Shared initialization
    void runHeadless();  // Run bot-vs-bot without GUI

public:
    void setWhiteBot(ChessBot* bot) { whiteBot = bot; }
    void setBlackBot(ChessBot* bot) { blackBot = bot; }
};
