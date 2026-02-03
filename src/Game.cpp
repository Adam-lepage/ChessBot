#include "Game.h"
#include "BitboardEngine.h"
#include <iostream>

Game::Game() 
    : window(sf::VideoMode::getDesktopMode(), "Chess Game", sf::Style::Default),
      board(),
      moveValidator(&board.getBitboardEngine()),
      isRunning(true),
      isFullscreen(false),
      boardView(sf::FloatRect(0, 0, BOARD_DISPLAY_SIZE, BOARD_DISPLAY_SIZE)),
      currentPlayer(WHITE),
      selectedRow(-1),
      selectedCol(-1),
      pieceSelected(false),
      isDragging(false),
      dragOffset(0, 0),
      isInCheck(false),
      isCheckmate(false),
      isGameOver(false) {
    window.setFramerateLimit(60);
    
    // Try to load a font - use system font if available
    font.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf");  // Linux/WSL
    if (!font.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf")) {
        font.loadFromFile("C:\\Windows\\Fonts\\arial.ttf");  // Windows fallback
    }
    
    std::cout << "White to move" << std::endl;
}

Game::~Game() {
    window.close();
}

void Game::run() {
    while (window.isOpen() && isRunning) {
        updateBoardView();  // Update view before input processing
        handleInput();
        update();
        render();
    }
}

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
                    std::cout << "[INPUT] Mouse pressed at screen (" << event.mouseButton.x << "," << event.mouseButton.y 
                              << ") world (" << worldPos.x << "," << worldPos.y << ")" << std::endl;
                    handleBoardClick(worldPos);
                }
                break;
            case sf::Event::MouseButtonReleased:
                if (event.mouseButton.button == sf::Mouse::Left && isDragging) {
                    sf::Vector2f worldPos = window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y), boardView);
                    std::cout << "[INPUT] Mouse released at screen (" << event.mouseButton.x << "," << event.mouseButton.y 
                              << ") world (" << worldPos.x << "," << worldPos.y << ")" << std::endl;
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

void Game::updateBoardView() {
    // Calculate the view to fit board with labels
    unsigned int windowWidth = window.getSize().x;
    unsigned int windowHeight = window.getSize().y;
    
    // Board size is 1024 (8 squares × 128px each)
    // With labels/padding it needs extra space: 60 pixels on each side
    int boardTotalSize = 1024 + 120;  // 8*128 + 120 for labels
    
    // Scale to fit window while maintaining aspect ratio
    float scaleX = static_cast<float>(windowWidth) / boardTotalSize;
    float scaleY = static_cast<float>(windowHeight) / boardTotalSize;
    float scale = std::min(scaleX, scaleY);
    
    // Don't scale up, only down
    if (scale > 1.0f) scale = 1.0f;
    
    int displayWidth = static_cast<int>(boardTotalSize * scale);
    int displayHeight = static_cast<int>(boardTotalSize * scale);
    
    // Center the board
    int offsetX = (windowWidth - displayWidth) / 2;
    int offsetY = (windowHeight - displayHeight) / 2;
    
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

void Game::update() {
    // Game logic will go here
}

void Game::render() {
    // Navy blue grayish background
    window.clear(sf::Color(40, 50, 70));
    
    // Set the pre-calculated view
    window.setView(boardView);
    
    // Draw the board
    board.draw(window, &font);
    
    // Draw move indicators for selected piece
    if (pieceSelected && validMoves.size() > 0) {
        board.drawMoveIndicators(window, validMoves);
    }
    
    // Reset view
    window.setView(window.getDefaultView());
    
    // Draw game over message if checkmate
    if (isGameOver && isCheckmate) {
        int winner = (currentPlayer == WHITE) ? BLACK : WHITE;
        std::string winnerName = (winner == WHITE) ? "White" : "Black";
        std::string message = winnerName + " wins by checkmate!";
        
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

void Game::handleBoardClick(sf::Vector2f worldPos) {
    // Constants for board layout
    const int BOARD_OFFSET = 60;
    const int SQUARE_SIZE = 128;
    
    std::cout << "[BOARD] Click detected at (" << worldPos.x << "," << worldPos.y << ")" << std::endl;
    
    // Check if click is within board
    if (worldPos.x < BOARD_OFFSET || worldPos.x > BOARD_OFFSET + 8 * SQUARE_SIZE ||
        worldPos.y < BOARD_OFFSET || worldPos.y > BOARD_OFFSET + 8 * SQUARE_SIZE) {
        std::cout << "[BOARD] Click outside board area" << std::endl;
        return;
    }
    
    // Calculate row and column
    int clickCol = static_cast<int>((worldPos.x - BOARD_OFFSET) / SQUARE_SIZE);
    int clickRow = static_cast<int>((worldPos.y - BOARD_OFFSET) / SQUARE_SIZE);
    
    std::cout << "[BOARD] Click at row=" << clickRow << " col=" << clickCol << std::endl;
    
    if (clickRow < 0 || clickRow > 7 || clickCol < 0 || clickCol > 7) {
        std::cout << "[BOARD] Invalid row/col" << std::endl;
        return;
    }
    
    // Check if there's a piece at this location
    int piece = moveValidator.getPieceAt(clickRow, clickCol);
    std::cout << "[BOARD] Piece at (" << clickRow << "," << clickCol << "): " << piece << std::endl;
    
    if (piece != -1) {
        int pieceColor = (piece % 2 == 0) ? WHITE : BLACK;
        if (pieceColor == currentPlayer) {
            // Start dragging this piece
            selectedRow = clickRow;
            selectedCol = clickCol;
            isDragging = true;
            pieceSelected = true;
            
            // Calculate drag offset (from piece center to click position)
            dragOffset = sf::Vector2f(worldPos.x - (BOARD_OFFSET + clickCol * SQUARE_SIZE + SQUARE_SIZE / 2),
                                     worldPos.y - (BOARD_OFFSET + clickRow * SQUARE_SIZE + SQUARE_SIZE / 2));
            
            // Calculate and display valid moves for this piece
            calculateValidMoves();
            
            std::cout << "[DRAG] Started dragging: " << BitboardEngine::getPieceChar(piece) 
                      << BitboardEngine::squareToAlgebraic(clickRow, clickCol) << std::endl;
        } else {
            std::cout << "[BOARD] Piece belongs to opponent" << std::endl;
        }
    } else {
        std::cout << "[BOARD] No piece at this square" << std::endl;
    }
}

void Game::updateDrag(sf::Vector2f worldPos) {
    // Update the piece position while dragging
    if (!isDragging) return;
    
    // Tell the board to update the dragging piece position
    board.setDraggedPiece(selectedRow, selectedCol, worldPos.x, worldPos.y);
}

void Game::completeDrag(sf::Vector2f worldPos) {
    std::cout << "[DEBUG] completeDrag called at (" << worldPos.x << "," << worldPos.y << ")" << std::endl;
    
    if (!isDragging) {
        std::cout << "[DEBUG] Not dragging!" << std::endl;
        return;
    }
    
    const int BOARD_OFFSET = 60;
    const int SQUARE_SIZE = 128;
    
    // Calculate target square
    if (worldPos.x < BOARD_OFFSET || worldPos.x > BOARD_OFFSET + 8 * SQUARE_SIZE ||
        worldPos.y < BOARD_OFFSET || worldPos.y > BOARD_OFFSET + 8 * SQUARE_SIZE) {
        // Dropped outside board - cancel move
        isDragging = false;
        pieceSelected = false;
        board.clearDraggedPiece();
        std::cout << "Move cancelled" << std::endl;
        return;
    }
    
    int targetCol = static_cast<int>((worldPos.x - BOARD_OFFSET) / SQUARE_SIZE);
    int targetRow = static_cast<int>((worldPos.y - BOARD_OFFSET) / SQUARE_SIZE);
    
    std::cout << "[DEBUG] Target: (" << targetRow << "," << targetCol << ")" << std::endl;
    
    if (targetRow < 0 || targetRow > 7 || targetCol < 0 || targetCol > 7) {
        isDragging = false;
        pieceSelected = false;
        board.clearDraggedPiece();
        return;
    }
    
    // Try to move the piece
    Move move(selectedRow, selectedCol, targetRow, targetCol);
    
    if (selectedRow == targetRow && selectedCol == targetCol) {
        // Same square - just deselect
        isDragging = false;
        pieceSelected = false;
        board.clearDraggedPiece();
        return;
    }
    
    std::cout << "[DEBUG] Executing move..." << std::endl;
    if (moveValidator.executeMove(move, currentPlayer)) {
        std::cout << "[DEBUG] Move validated, updating board..." << std::endl;
        // Update visual board
        board.movePieceOnBoard(selectedRow, selectedCol, targetRow, targetCol);
        
        std::cout << BitboardEngine::squareToAlgebraic(selectedRow, selectedCol) << " -> " 
                  << BitboardEngine::squareToAlgebraic(targetRow, targetCol) << std::endl;
        
        // Switch turns
        currentPlayer = (currentPlayer == WHITE) ? BLACK : WHITE;
        std::cout << ((currentPlayer == WHITE) ? "White" : "Black") << " to move" << std::endl;
        
        // Check for check/checkmate
        checkForCheckmate();
        
        // Print board state
        board.getBitboardEngine().printBoard();
    } else {
        std::cout << "Invalid move" << std::endl;
    }
    
    isDragging = false;
    validMoves.clear();  // Clear move indicators
    pieceSelected = false;
    board.clearDraggedPiece();
}

void Game::calculateValidMoves() {
    validMoves.clear();
    
    if (!pieceSelected || selectedRow < 0 || selectedCol < 0) {
        return;
    }
    
    // Get all valid moves for the selected piece
    std::vector<Move> moves = moveValidator.getValidMoves(selectedRow, selectedCol, currentPlayer);
    
    for (const auto& move : moves) {
        validMoves.push_back({move.toRow, move.toCol});
    }
    
    std::cout << "[MOVES] Found " << validMoves.size() << " valid moves" << std::endl;
}

void Game::checkForCheckmate() {
    // Check if current player is in checkmate
    if (moveValidator.isCheckmate(currentPlayer)) {
        isCheckmate = true;
        isGameOver = true;
        int winner = (currentPlayer == WHITE) ? BLACK : WHITE;
        std::cout << (winner == WHITE ? "White" : "Black") << " wins by checkmate!" << std::endl;
    } else if (moveValidator.isKingInCheck(currentPlayer)) {
        isInCheck = true;
        std::cout << (currentPlayer == WHITE ? "White" : "Black") << " is in check!" << std::endl;
    } else {
        isInCheck = false;
    }
}

void Game::restartGame() {
    // Reset game state
    isGameOver = false;
    isCheckmate = false;
    isInCheck = false;
    currentPlayer = WHITE;
    selectedRow = -1;
    selectedCol = -1;
    pieceSelected = false;
    isDragging = false;
    validMoves.clear();
    
    // Reinitialize board
    board.initializePieces();
    moveValidator.clearEnPassantSquare();
    
    std::cout << "Game restarted. White to move" << std::endl;
}
