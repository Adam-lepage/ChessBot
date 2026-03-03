#include "Game.h"
#include "BitboardEngine.h"
#include <iostream>
#include <iomanip>
#include <cmath>

bool g_debugOutput = false;

Game::Game(const GameConfig& cfg)
    : window(),
      board(!cfg.gui),
      moveValidator(&board.getBitboardEngine()),
      isRunning(true),
      isFullscreen(false),
      boardView(sf::FloatRect(0, 0, BOARD_DISPLAY_SIZE, BOARD_DISPLAY_SIZE)),
      currentPlayer(WHITE),
      selectedRow(-1),
      selectedCol(-1),
      pieceSelected(false),
      isInCheck(false),
      isCheckmate(false),
      isStalemate(false),
    isDrawByMoveLimit(false),
    isDrawByMaterial(false),
    isDrawByRepetition(false),
      isGameOver(false),
    isDragging(false),
    waitingForPromotion(false),
    pendingPromotionMove(0, 0, 0, 0),
    promotionCol(-1),
      whiteBot(nullptr),
      blackBot(nullptr),
    halfmoveClock(0),
    fullMoveNumber(1),
    boardScreenLeft(0), boardScreenTop(0), boardScreenRight(0), boardScreenBottom(0),
    turnTimerRunning(false),
    headless(!cfg.gui),
      config(cfg) {
    g_debugOutput = cfg.debug;
    init();
}

// creates sfml window if needed 
void Game::init() {
    if (!headless) {
        window.create(sf::VideoMode::getDesktopMode(), "Chess Game", sf::Style::Default);
        window.setFramerateLimit(60);
        
        // Try to load a font - Linux/WSL first, then Windows fallback
        if (!font.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf")) {
            font.loadFromFile("C:\\Windows\\Fonts\\arial.ttf");
        }
    }
    
    if (g_debugOutput) {
        std::cout << "White to move" << std::endl;
    }
    startTurnTimer();
    if (g_debugOutput) {
        std::cout << "[DEBUG] Debug output enabled" << std::endl;
        std::cout << "[DEBUG] Mode: ";
        switch (config.mode) {
            case GameMode::PVP: std::cout << "Player vs Player"; break;
            case GameMode::PVB: std::cout << "Player vs Bot"; break;
            case GameMode::BVB: std::cout << "Bot vs Bot"; break;
        }
        std::cout << std::endl;
        std::cout << "[DEBUG] GUI: " << (headless ? "off" : "on") << std::endl;
    }
}

Game::~Game() {
    if (window.isOpen()) {
        window.close();
    }
}

// Main game loop
void Game::run() {
    if (headless) {
        runHeadless();
        return;
    }
    while (window.isOpen() && isRunning) {
        updateBoardView();  // Update view before input processing
        handleInput();
        update();
        render();
    }
}

// only processes moves for bots, no GUI or input handling
void Game::runHeadless() {
    // Headless bot-vs-bot loop — no GUI, just console output
    while (isRunning && !isGameOver) {
        if (!isBotTurn()) {
            std::cerr << "Error: headless mode requires both players to be bots." << std::endl;
            break;
        }
        processBotMove();
    }
}

// Get inputs from sfml window and handles them accordingly
void Game::handleInput() {
    sf::Event event;
    while (window.pollEvent(event)) {
        switch (event.type) {
            case sf::Event::Closed:
                window.close();
                isRunning = false;
                break;
            case sf::Event::KeyPressed:
                if (event.key.code == sf::Keyboard::Escape) {
                    window.close();
                    isRunning = false;
                }
                else if (event.key.code == sf::Keyboard::R && isGameOver) {
                    // Restart game
                    restartGame();
                }
                else if (event.key.code == sf::Keyboard::F) {
                    // Toggle fullscreen
                    isFullscreen = !isFullscreen;
                    if (isFullscreen) {
                        window.create(sf::VideoMode::getDesktopMode(), "Chess Game", sf::Style::Fullscreen);
                    } else {
                        window.create(sf::VideoMode(1024, 1024), "Chess Game", sf::Style::Default);
                    }
                    window.setFramerateLimit(60);
                }
                break;
            case sf::Event::MouseButtonPressed:
                if (event.mouseButton.button == sf::Mouse::Left) {
                    sf::Vector2f worldPos = window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y), boardView);
                    if (waitingForPromotion) {
                        handlePromotionClick(worldPos);
                    } else {
                        handleBoardClick(worldPos);
                    }
                }
                break;
            case sf::Event::MouseButtonReleased:
                if (event.mouseButton.button == sf::Mouse::Left && isDragging) {
                    sf::Vector2f worldPos = window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y), boardView);
                    completeDrag(worldPos);
                }
                break;
            case sf::Event::MouseMoved:
                if (isDragging) {
                    sf::Vector2f worldPos = window.mapPixelToCoords(sf::Vector2i(event.mouseMove.x, event.mouseMove.y), boardView);
                    updateDrag(worldPos);
                }
                break;
            default:
                break;
        }
    }
}

// Update the view to fit the board with labels
void Game::updateBoardView() {
    // Calculate the view to fit board with labels
    unsigned int windowWidth = window.getSize().x;
    unsigned int windowHeight = window.getSize().y;
    
    // Board size is 1024 (8 squares × 128px each)
    int boardTotalSize = 1024 + 120;  // 8*128 + 120 for labels
    
    // Reserve left margin for captured pieces panel
    int leftPanelWidth = std::max(120, static_cast<int>(windowWidth * 0.12f));
    int availableWidth = static_cast<int>(windowWidth) - leftPanelWidth;
    
    // Scale to fit window while maintaining aspect ratio
    float scaleX = static_cast<float>(availableWidth) / boardTotalSize;
    float scaleY = static_cast<float>(windowHeight) / boardTotalSize;
    float scale = std::min(scaleX, scaleY);
    
    // Don't scale up, only down
    if (scale > 1.0f) scale = 1.0f;
    
    int displayWidth = static_cast<int>(boardTotalSize * scale);
    int displayHeight = static_cast<int>(boardTotalSize * scale);
    
    // Position board to the right of the left panel, centered vertically
    int offsetX = leftPanelWidth + (availableWidth - displayWidth) / 2;
    int offsetY = (windowHeight - displayHeight) / 2;
    
    // Store board viewport position in window pixels for overlay drawing
    boardScreenLeft = static_cast<float>(offsetX);
    boardScreenTop = static_cast<float>(offsetY);
    boardScreenRight = static_cast<float>(offsetX + displayWidth);
    boardScreenBottom = static_cast<float>(offsetY + displayHeight);
    
    // Create view that shows the entire board with labels
    boardView.setSize(boardTotalSize, boardTotalSize);
    boardView.setCenter(boardTotalSize / 2.0f, boardTotalSize / 2.0f);
    
    sf::FloatRect viewport(
        static_cast<float>(offsetX) / windowWidth,
        static_cast<float>(offsetY) / windowHeight,
        static_cast<float>(displayWidth) / windowWidth,
        static_cast<float>(displayHeight) / windowHeight
    );
    boardView.setViewport(viewport);
}

// If it is a bot's turn, render the current board state first so the
// player's move is visually placed, then process the bot's move.
void Game::update() {
    if (!isGameOver && isBotTurn()) {
        // Render one frame so the player's last move is visible before the bot blocks
        render();
        processBotMove();
    }
}

// Draw the board, pieces, move indicators, and promotion UI if needed
void Game::render() {
    // Navy blue grayish background
    window.clear(sf::Color(40, 50, 70));
    
    // Set the pre-calculated view
    window.setView(boardView);
    
    // Draw the board
    board.draw(window, &font);
    
    // Draw selected square highlight (darkened square under the piece)
    if (pieceSelected) {
        board.drawSelectedSquare(window, selectedRow, selectedCol);
    }
    
    // Draw move indicators for selected piece
    if (pieceSelected && validMoves.size() > 0) {
        board.drawMoveIndicators(window, validMoves);
    }
    
    // Draw promotion UI if waiting for choice
    if (waitingForPromotion) {
        board.drawPromotionUI(window, promotionCol, currentPlayer);
    }
    
    // Reset view for overlay drawing (captured pieces, move number, game over)
    window.setView(window.getDefaultView());
    
    // Draw captured pieces on the left side
    drawCapturedPieces();
    
    // Draw move number in top-right
    drawMoveNumber();
    
    // Draw game over message
    if (isGameOver) {
        std::string message;
        if (isCheckmate) {
            int winner = (currentPlayer == WHITE) ? BLACK : WHITE;
            std::string winnerName = (winner == WHITE) ? "White" : "Black";
            message = winnerName + " wins by checkmate!";
        } else if (isStalemate) {
            message = "Draw by stalemate!";
        } else if (isDrawByMaterial) {
            message = "Draw by insufficient material!";
        } else if (isDrawByMoveLimit) {
            message = "Draw by 75-move rule!";
        } else if (isDrawByRepetition) {
            message = "Draw by threefold repetition!";
        }
        
        // Create a semi-transparent overlay
        sf::RectangleShape overlay(sf::Vector2f(window.getSize().x, window.getSize().y));
        overlay.setFillColor(sf::Color(0, 0, 0, 200));
        window.draw(overlay);
        
        // Create winner text
        sf::Text winnerText(message, font, 60);
        winnerText.setFillColor(sf::Color::Yellow);
        
        // Center the text
        sf::FloatRect textBounds = winnerText.getLocalBounds();
        winnerText.setPosition(
            (window.getSize().x - textBounds.width) / 2.0f,
            (window.getSize().y - textBounds.height) / 2.0f - 40
        );
        
        window.draw(winnerText);
        
        // Draw instructions
        sf::Text instructionsText("Press ESC to close or R to restart", font, 24);
        instructionsText.setFillColor(sf::Color::White);
        
        sf::FloatRect instBounds = instructionsText.getLocalBounds();
        instructionsText.setPosition(
            (window.getSize().x - instBounds.width) / 2.0f,
            (window.getSize().y - instBounds.height) / 2.0f + 50
        );
        
        window.draw(instructionsText);
    }
    
    window.display();
}

// Handle a click on the board for selecting/moving pieces
void Game::handleBoardClick(sf::Vector2f worldPos) {
    if (isGameOver || isBotTurn() || waitingForPromotion) return;
    
    // Constants for board layout
    const int BOARD_OFFSET = 60;
    const int SQUARE_SIZE = 128;
    
    // Check if click is within board
    if (worldPos.x < BOARD_OFFSET || worldPos.x > BOARD_OFFSET + 8 * SQUARE_SIZE ||
        worldPos.y < BOARD_OFFSET || worldPos.y > BOARD_OFFSET + 8 * SQUARE_SIZE) {
        // Clicked outside board — deselect
        pieceSelected = false;
        validMoves.clear();
        return;
    }
    
    // Calculate row and column
    int clickCol = static_cast<int>((worldPos.x - BOARD_OFFSET) / SQUARE_SIZE);
    int clickRow = static_cast<int>((worldPos.y - BOARD_OFFSET) / SQUARE_SIZE);
    
    if (clickRow < 0 || clickRow > 7 || clickCol < 0 || clickCol > 7) {
        return;
    }
    
    int piece = moveValidator.getPieceAt(clickRow, clickCol);
    bool isOwnPiece = (piece != -1) && ((piece % 2 == 0 ? WHITE : BLACK) == currentPlayer);
    
    if (pieceSelected) {
        // A piece is already selected
        
        if (clickRow == selectedRow && clickCol == selectedCol) {
            // Clicked the same piece again — deselect
            pieceSelected = false;
            validMoves.clear();
            return;
        }
        
        // Check if clicking a valid move destination
        bool isValidDest = false;
        for (const auto& [r, c, cap] : validMoves) {
            if (r == clickRow && c == clickCol) {
                isValidDest = true;
                break;
            }
        }
        
        if (isValidDest) {
            // Execute the move via click
            executePlayerMove(clickRow, clickCol);
            return;
        }
        
        if (isOwnPiece) {
            // Clicked a different own piece, select it and start drag
            selectedRow = clickRow;
            selectedCol = clickCol;
            pieceSelected = true;
            isDragging = true;
            calculateValidMoves();
            return;
        }
        
        // Clicked an invalid square — deselect
        pieceSelected = false;
        validMoves.clear();
        return;
    }
    
    // No piece currently selected
    if (isOwnPiece) {
        // Select the piece and start drag
        selectedRow = clickRow;
        selectedCol = clickCol;
        isDragging = true;
        pieceSelected = true;
        
        calculateValidMoves();
    }
}

void Game::updateDrag(sf::Vector2f worldPos) {
    // Update the piece position while dragging
    if (!isDragging) return;
    
    // Tell the board to update the dragging piece position
    board.setDraggedPiece(selectedRow, selectedCol, worldPos.x, worldPos.y);
}

// Handle dropping the piece after dragging
void Game::completeDrag(sf::Vector2f worldPos) {
    if (!isDragging) return;
    
    const int BOARD_OFFSET = 60;
    const int SQUARE_SIZE = 128;
    
    // Calculate target square
    if (worldPos.x < BOARD_OFFSET || worldPos.x > BOARD_OFFSET + 8 * SQUARE_SIZE ||
        worldPos.y < BOARD_OFFSET || worldPos.y > BOARD_OFFSET + 8 * SQUARE_SIZE) {
        // Dropped outside board — stop drag but keep selected for click-to-move
        isDragging = false;
        board.clearDraggedPiece();
        return;
    }
    
    int targetCol = static_cast<int>((worldPos.x - BOARD_OFFSET) / SQUARE_SIZE);
    int targetRow = static_cast<int>((worldPos.y - BOARD_OFFSET) / SQUARE_SIZE);
    
    if (targetRow < 0 || targetRow > 7 || targetCol < 0 || targetCol > 7) {
        isDragging = false;
        board.clearDraggedPiece();
        return;
    }
    
    if (selectedRow == targetRow && selectedCol == targetCol) {
        // Dropped on same square — stop drag but keep piece selected (click-to-move mode)
        isDragging = false;
        board.clearDraggedPiece();
        return;
    }
    
    // Try to move the piece via drag
    isDragging = false;
    board.clearDraggedPiece();
    executePlayerMove(targetRow, targetCol);
}

// Handles all player moves
void Game::executePlayerMove(int targetRow, int targetCol) {
    // Check if this is a promotion move — if so, show UI and defer execution
    int piece = moveValidator.getPieceAt(selectedRow, selectedCol);
    bool isPromotion = (piece / 2 == 0) &&
        ((currentPlayer == WHITE && targetRow == 0) ||
         (currentPlayer == BLACK && targetRow == 7));
    
    if (isPromotion) {
        // Validate the move first (without executing)
        if (!moveValidator.isValidMove(selectedRow, selectedCol, targetRow, targetCol, currentPlayer)) {
            validMoves.clear();
            pieceSelected = false;
            return;
        }
        
        // Enter promotion state — wait for user to pick a piece
        waitingForPromotion = true;
        pendingPromotionMove = Move(selectedRow, selectedCol, targetRow, targetCol);
        promotionCol = targetCol;
        isDragging = false;
        board.clearDraggedPiece();
        validMoves.clear();
        pieceSelected = false;
        return;
    }
    
    Move move(selectedRow, selectedCol, targetRow, targetCol);
    
    if (moveValidator.executeMove(move, currentPlayer)) {
        // Record turn time before switching players
        stopTurnTimer();
        recordCapture(move);

        std::cout << BitboardEngine::squareToAlgebraic(selectedRow, selectedCol) << " -> " 
                  << BitboardEngine::squareToAlgebraic(targetRow, targetCol);
        if (move.isCastling) std::cout << " (castle)";
        std::cout << std::endl;
        
        // Update 75-move clock: reset on capture or pawn move
        int movedPiece = moveValidator.getPieceAt(targetRow, targetCol);
        if (move.capturedPiece != -1 || move.isPawnPromotion ||
            movedPiece == BitboardEngine::WHITE_PAWN || movedPiece == BitboardEngine::BLACK_PAWN) {
            halfmoveClock = 0;
        } else {
            halfmoveClock++;
        }

        // Switch turns
        currentPlayer = (currentPlayer == WHITE) ? BLACK : WHITE;
        if (currentPlayer == WHITE) fullMoveNumber++;  // Increment after black moves
        std::cout << ((currentPlayer == WHITE) ? "White" : "Black") << " to move" << std::endl;

        // Check for check/checkmate and draw conditions
        checkForCheckmate();
        if (!isGameOver) {
            checkForDrawConditions(move);
        }
        if (isGameOver) {
            printTurnTimeStats();
        } else {
            startTurnTimer();
        }
        
    } else {
        if (g_debugOutput) {
            std::cout << "[DEBUG] Invalid move" << std::endl;
        }
    }
    
    validMoves.clear();
    pieceSelected = false;
}

void Game::handlePromotionClick(sf::Vector2f worldPos) {
    int choice = board.getPromotionChoice(worldPos.x, worldPos.y, promotionCol, currentPlayer);
    if (choice != -1) {
        completePromotion(choice);
    }
    // If clicked outside options, do nothing (keep waiting)
}

// Complete a pawn promotion after the user has selected the piece to promote to
void Game::completePromotion(int promotedPiece) {
    pendingPromotionMove.promotedTo = promotedPiece;
    
    if (moveValidator.executeMove(pendingPromotionMove, currentPlayer)) {
        // Record turn time before switching players
        stopTurnTimer();
        recordCapture(pendingPromotionMove);

        std::cout << BitboardEngine::squareToAlgebraic(pendingPromotionMove.fromRow, pendingPromotionMove.fromCol) << " -> "
                  << BitboardEngine::squareToAlgebraic(pendingPromotionMove.toRow, pendingPromotionMove.toCol)
                  << " (promotion)" << std::endl;
        
        // Promotion is always a pawn move, so reset halfmove clock
        halfmoveClock = 0;
        
        currentPlayer = (currentPlayer == WHITE) ? BLACK : WHITE;
        if (currentPlayer == WHITE) fullMoveNumber++;  // Increment after black moves
        std::cout << ((currentPlayer == WHITE) ? "White" : "Black") << " to move" << std::endl;
        
        checkForCheckmate();
        if (!isGameOver) {
            checkForDrawConditions(pendingPromotionMove);
        }
        if (isGameOver) {
            printTurnTimeStats();
        } else {
            startTurnTimer();
        }
    }
    
    waitingForPromotion = false;
    promotionCol = -1;
}

// Gets all legal moves for the currently selected piece and stores them for rendering move indicators
void Game::calculateValidMoves() {
    validMoves.clear();
    
    if (!pieceSelected || selectedRow < 0 || selectedCol < 0) {
        return;
    }
    
    // Get all valid moves for the selected piece
    std::vector<Move> moves = moveValidator.getValidMoves(selectedRow, selectedCol, currentPlayer);
    
    for (const auto& move : moves) {
        // Check if this move is a capture (enemy piece on target or en passant)
        int targetPiece = moveValidator.getPieceAt(move.toRow, move.toCol);
        bool isCapture = (targetPiece != -1) || move.isEnPassant;
        validMoves.push_back({move.toRow, move.toCol, isCapture});
    }
}

// Checks if the current player is in checkmate, stalemate, or just check, and updates game state accordingly
void Game::checkForCheckmate() {
    isCheckmate = false;
    isStalemate = false;
    isInCheck = false;
    bool inCheck = moveValidator.isKingInCheck(currentPlayer);
    bool hasLegalMoves = moveValidator.hasAnyLegalMoves(currentPlayer);
    
    if (!hasLegalMoves) {
        isGameOver = true;
        if (inCheck) {
            isCheckmate = true;
            int winner = (currentPlayer == WHITE) ? BLACK : WHITE;
            std::string winnerLabel = (winner == WHITE) ? "White" : "Black";

            ChessBot* winnerBot = (winner == WHITE) ? whiteBot : blackBot;
            if (winnerBot) {
                winnerLabel += " (" + winnerBot->getName() + ")";
            }

            if (g_debugOutput) {
                std::cout << winnerLabel << " wins by checkmate!" << std::endl;
            }
        } else {
            isStalemate = true;
            if (g_debugOutput) {
                std::cout << "Draw by stalemate!" << std::endl;
            }
        }
    } else if (inCheck) {
        isInCheck = true;
        if (g_debugOutput) {
            std::cout << (currentPlayer == WHITE ? "White" : "Black") << " is in check!" << std::endl;
        }
    } else {
        isInCheck = false;
    }
}

// Checks for draw conditions: insufficient material, 75-move rule, or move repetition
void Game::checkForDrawConditions(const Move& lastMove) {
    if (onlyKingsLeft()) {
        isGameOver = true;
        isDrawByMaterial = true;
        if (g_debugOutput) {
            std::cout << "Draw by insufficient material!" << std::endl;
        }
        return;
    }

    if (halfmoveClock >= 150) {  // 75 full moves = 150 half-moves
        isGameOver = true;
        isDrawByMoveLimit = true;
        if (g_debugOutput) {
            std::cout << "Draw by 75-move rule!" << std::endl;
        }
        return;
    }

    // Move repetition: detect pieces shuffling back and forth
    // A "cycle" is 4 half-moves: white goes A->B, black goes X->Y, white goes B->A, black goes Y->X
    // If we see 2 full cycles (8 half-moves) with the same pattern, it's a draw
    moveHistory.push_back(lastMove);
    size_t n = moveHistory.size();
    if (n >= 8) {
        auto same = [](const Move& a, const Move& b) {
            return a.fromRow == b.fromRow && a.fromCol == b.fromCol &&
                   a.toRow == b.toRow && a.toCol == b.toCol;
        };
        // Check if half-moves [n-8..n-5] == [n-4..n-1]  (same 4-move cycle twice)
        bool cycleRepeats = true;
        for (int i = 0; i < 4; i++) {
            if (!same(moveHistory[n-8+i], moveHistory[n-4+i])) {
                cycleRepeats = false;
                break;
            }
        }
        if (cycleRepeats) {
            isGameOver = true;
            isDrawByRepetition = true;
            if (g_debugOutput) {
                std::cout << "Draw by move repetition!" << std::endl;
            }
        }
    }
}

// Returns true if only the two kings are left on the board
bool Game::onlyKingsLeft() const {
    const BitboardEngine& engine = board.getBitboardEngine();
    if (engine.kings[0] == 0 || engine.kings[1] == 0) {
        return false;
    }
    Bitboard onlyKings = engine.kings[0] | engine.kings[1];
    return engine.allPieces == onlyKings;
}

// Restart the game by resetting all states and reinitializing the board
void Game::restartGame() {
    // Reset game state
    isGameOver = false;
    isCheckmate = false;
    isStalemate = false;
    isDrawByMoveLimit = false;
    isDrawByMaterial = false;
    isDrawByRepetition = false;
    isInCheck = false;
    currentPlayer = WHITE;
    selectedRow = -1;
    selectedCol = -1;
    pieceSelected = false;
    isDragging = false;
    waitingForPromotion = false;
    pendingPromotionMove = Move(0, 0, 0, 0);
    promotionCol = -1;
    validMoves.clear();
    halfmoveClock = 0;
    moveHistory.clear();
    whiteTurnTimes.clear();
    blackTurnTimes.clear();
    turnTimerRunning = false;
    capturedByWhite.clear();
    capturedByBlack.clear();
    fullMoveNumber = 1;
    
    // Reinitialize board
    board.initializePieces();
    moveValidator.clearEnPassantSquare();
    moveValidator.resetCastlingRights();
    
    std::cout << "Game restarted. White to move" << std::endl;
    startTurnTimer();
}

// Returns true if it's currently a bot's turn to move
bool Game::isBotTurn() const {
    if (currentPlayer == WHITE && whiteBot != nullptr) return true;
    if (currentPlayer == BLACK && blackBot != nullptr) return true;
    return false;
}

// Process a move for the current bot player
void Game::processBotMove() {
    ChessBot* bot = (currentPlayer == WHITE) ? whiteBot : blackBot;
    if (!bot) return;
    
    Move move = bot->chooseMove(board.getBitboardEngine(), moveValidator, currentPlayer);
    
    if (moveValidator.executeMove(move, currentPlayer)) {
        // Record turn time before switching players
        stopTurnTimer();
        recordCapture(move);

        if (g_debugOutput) {
            std::cout << bot->getName() << ": "
                      << BitboardEngine::squareToAlgebraic(move.fromRow, move.fromCol) << " -> " 
                      << BitboardEngine::squareToAlgebraic(move.toRow, move.toCol);
            if (move.isCastling) std::cout << " (castle)";
            if (move.isEnPassant) std::cout << " (en passant)";
            if (move.isPawnPromotion) std::cout << " (promotion)";
            if (move.capturedPiece != -1) std::cout << " (capture)";
            std::cout << std::endl;
        }
        
        // Update 75-move clock: reset on capture or pawn move
        int movedPiece = moveValidator.getPieceAt(move.toRow, move.toCol);
        if (move.capturedPiece != -1 || move.isPawnPromotion ||
            movedPiece == BitboardEngine::WHITE_PAWN || movedPiece == BitboardEngine::BLACK_PAWN) {
            halfmoveClock = 0;
        } else {
            halfmoveClock++;
        }

        currentPlayer = (currentPlayer == WHITE) ? BLACK : WHITE;
        if (currentPlayer == WHITE) fullMoveNumber++;  // Increment after black moves
        
        if (g_debugOutput) {
            std::cout << ((currentPlayer == WHITE) ? "White" : "Black") << " to move" << std::endl;
        }

        checkForCheckmate();
        if (!isGameOver) {
            checkForDrawConditions(move);
        }
        if (isGameOver) {
            printTurnTimeStats();
        } else {
            startTurnTimer();
        }
    }
}

void Game::startTurnTimer() {
    turnStartTime = std::chrono::steady_clock::now();
    turnTimerRunning = true;
}

void Game::stopTurnTimer() {
    if (!turnTimerRunning) return;
    auto now = std::chrono::steady_clock::now();
    double seconds = std::chrono::duration<double>(now - turnStartTime).count();
    if (seconds < 0.0) seconds = 0.0;  // Guard against clock anomalies
    if (currentPlayer == WHITE) {
        whiteTurnTimes.push_back(seconds);
    } else {
        blackTurnTimes.push_back(seconds);
    }
    turnTimerRunning = false;
}

void Game::printTurnTimeStats() const {
    auto avg = [](const std::vector<double>& times) -> double {
        if (times.empty()) return 0.0;
        double sum = 0.0;
        for (double t : times) sum += t;
        return sum / times.size();
    };

    if (g_debugOutput) {
        std::cout << "\n=== Turn Time Stats ===" << std::endl;

        std::string whiteLabel = "White";
        std::string blackLabel = "Black";
        if (whiteBot) whiteLabel += " (" + whiteBot->getName() + ")";
        if (blackBot) blackLabel += " (" + blackBot->getName() + ")";

        std::cout << "  " << whiteLabel << ": " << whiteTurnTimes.size() << " moves, avg "
                  << std::fixed << std::setprecision(3) << avg(whiteTurnTimes) << "s per turn" << std::endl;
        std::cout << "  " << blackLabel << ": " << blackTurnTimes.size() << " moves, avg "
                  << std::fixed << std::setprecision(3) << avg(blackTurnTimes) << "s per turn" << std::endl;
        std::cout << std::endl;
    }
}

void Game::recordCapture(const Move& move) {
    if (move.capturedPiece == -1) return;
    // Determine which side made the capture based on currentPlayer
    // (currentPlayer hasn't switched yet when this is called)
    if (currentPlayer == WHITE) {
        capturedByWhite.push_back(move.capturedPiece);  // White captured a black piece
    } else {
        capturedByBlack.push_back(move.capturedPiece);  // Black captured a white piece
    }
}

void Game::drawCapturedPieces() {
    if (headless) return;
    
    // Piece value order for sorting (higher value first): Queen, Rook, Bishop, Knight, Pawn
    auto pieceValue = [](int pieceType) -> int {
        switch (pieceType / 2) {
            case 4: return 5;  // Queen
            case 1: return 4;  // Rook
            case 3: return 3;  // Bishop
            case 2: return 2;  // Knight
            case 0: return 1;  // Pawn
            default: return 0;
        }
    };
    
    float panelRight = boardScreenLeft - 10.0f;  // Right edge of the panel (10px gap from board)
    if (panelRight < 50.0f) return;  // Not enough space
    
    float pieceSize = 70.0f;//std::min(35.0f, (panelRight - 10.0f) / 5.0f);  // Size per piece icon
    float piecesPerRow = std::max(1.0f, std::floor((panelRight - 10.0f) / pieceSize));
    float startX = 10.0f;  // Left margin
    
    // Draw captured BLACK pieces at the TOP (black's side)
    {
        std::vector<int> sorted = capturedByBlack;  // Black pieces that white captured
        std::sort(sorted.begin(), sorted.end(), [&](int a, int b) {
            return pieceValue(a) > pieceValue(b);
        });
        
        float y = boardScreenTop + 10.0f;
        float x = startX;
        int col = 0;
        for (int piece : sorted) {
            board.drawPieceSprite(window, piece, x + pieceSize / 2.0f, y + pieceSize / 2.0f, pieceSize / 128.0f);
            col++;
            if (col >= static_cast<int>(piecesPerRow)) {
                col = 0;
                x = startX;
                y += pieceSize;
            } else {
                x += pieceSize;
            }
        }
    }
    
    // Draw captured WHITE pieces at the BOTTOM (white's side)
    {
        std::vector<int> sorted = capturedByWhite;  // White pieces that black captured
        std::sort(sorted.begin(), sorted.end(), [&](int a, int b) {
            return pieceValue(a) > pieceValue(b);
        });
        
        // Calculate total rows needed to position from bottom up
        int totalPieces = static_cast<int>(sorted.size());
        int rows = (totalPieces > 0) ? static_cast<int>(std::ceil(static_cast<float>(totalPieces) / piecesPerRow)) : 0;
        float startY = boardScreenBottom - 10.0f - rows * pieceSize;
        
        float y = startY;
        float x = startX;
        int col = 0;
        for (int piece : sorted) {
            board.drawPieceSprite(window, piece, x + pieceSize / 2.0f, y + pieceSize / 2.0f, pieceSize / 128.0f);
            col++;
            if (col >= static_cast<int>(piecesPerRow)) {
                col = 0;
                x = startX;
                y += pieceSize;
            } else {
                x += pieceSize;
            }
        }
    }
}

void Game::drawMoveNumber() {
    if (headless) return;
    
    // Calculate the current move display: "Move X" where X is the full move number
    // During white's turn, it's move N. During black's turn, it's still move N.
    std::string moveText = "Move " + std::to_string(fullMoveNumber);
    
    sf::Text text(moveText, font, 22);
    text.setFillColor(sf::Color::White);
    
    sf::FloatRect bounds = text.getLocalBounds();
    // Position in the top-right of the window
    text.setPosition(window.getSize().x - bounds.width - 20.0f, 15.0f);
    window.draw(text);
}

