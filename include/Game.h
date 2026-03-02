#pragma once

#include <SFML/Graphics.hpp>
#include "Board.h"
#include "MoveValidator.h"
#include "ChessBot.h"
#include "GameConfig.h"
#include <vector>
#include <chrono>

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
    bool isDrawByRepetition;
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

    // Move repetition tracking (same moves played 3 times in a row)
    std::vector<Move> moveHistory;

    // Turn timing
    std::chrono::high_resolution_clock::time_point turnStartTime;
    std::vector<double> whiteTurnTimes; // seconds per white turn
    std::vector<double> blackTurnTimes; // seconds per black turn
    bool turnTimerRunning;
    
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
    void checkForDrawConditions(const Move& lastMove);  // Check for draw conditions (material/move limit/repetition)
    bool onlyKingsLeft() const;  // True when only two kings remain
    void restartGame();  // Restart the game
    bool isBotTurn() const;  // Check if current player is a bot
    void processBotMove();  // Execute bot's chosen move
    void init();  // Shared initialization
    void runHeadless();  // Run bot-vs-bot without GUI
    void startTurnTimer();  // Start timing the current turn
    void stopTurnTimer();   // Stop timing and record the elapsed time
    void printTurnTimeStats() const; // Print average turn times at end of game

public:
    // Game result: 0 = white wins, 1 = black wins, 2 = draw
    enum GameResult { WHITE_WIN = 0, BLACK_WIN = 1, DRAW = 2 };

    void setWhiteBot(ChessBot* bot) { whiteBot = bot; }
    void setBlackBot(ChessBot* bot) { blackBot = bot; }

    // Returns the result after the game is over
    GameResult getGameResult() const {
        if (isCheckmate) {
            // currentPlayer is the one who has no moves (lost)
            return (currentPlayer == WHITE) ? BLACK_WIN : WHITE_WIN;
        }
        return DRAW;
    }
};
