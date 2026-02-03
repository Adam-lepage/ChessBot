#include "Board.h"
#include <iostream>

Board::Board() 
    : lightSquareColor(240, 217, 181),  // Light tan
      darkSquareColor(181, 136, 99),   // Dark brown
      draggedRow(-1),
      draggedCol(-1),
      draggedScreenX(0),
      draggedScreenY(0),
      isDraggingPiece(false) {
    initializePieces();
    bitboardEngine.printBoard();
}

Board::~Board() {
}

void Board::initializePieces() {
    // Clear the board
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            squares[i][j] = nullptr;
        }
    }

    // Setup white pieces (bottom - row 7 and 6)
    // White pawns
    for (int col = 0; col < BOARD_SIZE; col++) {
        squares[6][col] = std::make_unique<Piece>(PieceType::PAWN, PieceColor::WHITE, 6, col);
    }

    // White back rank (row 7)
    squares[7][0] = std::make_unique<Piece>(PieceType::ROOK, PieceColor::WHITE, 7, 0);
    squares[7][1] = std::make_unique<Piece>(PieceType::KNIGHT, PieceColor::WHITE, 7, 1);
    squares[7][2] = std::make_unique<Piece>(PieceType::BISHOP, PieceColor::WHITE, 7, 2);
    squares[7][3] = std::make_unique<Piece>(PieceType::QUEEN, PieceColor::WHITE, 7, 3);
    squares[7][4] = std::make_unique<Piece>(PieceType::KING, PieceColor::WHITE, 7, 4);
    squares[7][5] = std::make_unique<Piece>(PieceType::BISHOP, PieceColor::WHITE, 7, 5);
    squares[7][6] = std::make_unique<Piece>(PieceType::KNIGHT, PieceColor::WHITE, 7, 6);
    squares[7][7] = std::make_unique<Piece>(PieceType::ROOK, PieceColor::WHITE, 7, 7);

    // Setup black pieces (top - row 0 and 1)
    // Black pawns
    for (int col = 0; col < BOARD_SIZE; col++) {
        squares[1][col] = std::make_unique<Piece>(PieceType::PAWN, PieceColor::BLACK, 1, col);
    }

    // Black back rank (row 0)
    squares[0][0] = std::make_unique<Piece>(PieceType::ROOK, PieceColor::BLACK, 0, 0);
    squares[0][1] = std::make_unique<Piece>(PieceType::KNIGHT, PieceColor::BLACK, 0, 1);
    squares[0][2] = std::make_unique<Piece>(PieceType::BISHOP, PieceColor::BLACK, 0, 2);
    squares[0][3] = std::make_unique<Piece>(PieceType::QUEEN, PieceColor::BLACK, 0, 3);
    squares[0][4] = std::make_unique<Piece>(PieceType::KING, PieceColor::BLACK, 0, 4);
    squares[0][5] = std::make_unique<Piece>(PieceType::BISHOP, PieceColor::BLACK, 0, 5);
    squares[0][6] = std::make_unique<Piece>(PieceType::KNIGHT, PieceColor::BLACK, 0, 6);
    squares[0][7] = std::make_unique<Piece>(PieceType::ROOK, PieceColor::BLACK, 0, 7);
}

void Board::draw(sf::RenderWindow& window, const sf::Font* font) {
    drawSquares(window);
    drawPieces(window);
    drawBoardLabels(window, font);
}

void Board::drawSquares(sf::RenderWindow& window) {
    for (int row = 0; row < BOARD_SIZE; row++) {
        for (int col = 0; col < BOARD_SIZE; col++) {
            sf::RectangleShape square(sf::Vector2f(SQUARE_SIZE, SQUARE_SIZE));
            square.setPosition(BOARD_OFFSET + col * SQUARE_SIZE, BOARD_OFFSET + row * SQUARE_SIZE);
            
            // Alternate colors in checkerboard pattern
            if ((row + col) % 2 == 0) {
                square.setFillColor(lightSquareColor);
            } else {
                square.setFillColor(darkSquareColor);
            }
            
            window.draw(square);
        }
    }
}

void Board::drawPieces(sf::RenderWindow& window) {
    for (int row = 0; row < BOARD_SIZE; row++) {
        for (int col = 0; col < BOARD_SIZE; col++) {
            if (squares[row][col] != nullptr) {
                // Skip drawing the dragged piece here - it will be drawn separately on top
                if (isDraggingPiece && row == draggedRow && col == draggedCol) {
                    continue;
                }
                squares[row][col]->draw(window, SQUARE_SIZE, BOARD_OFFSET, BOARD_OFFSET);
            }
        }
    }
    
    // Draw the dragged piece on top at screen position
    if (isDraggingPiece && squares[draggedRow][draggedCol] != nullptr) {
        // Draw directly at the dragged position without normal offset
        squares[draggedRow][draggedCol]->drawAtPosition(window, draggedScreenX, draggedScreenY);
    }
}

Piece* Board::getPieceAt(int row, int col) {
    if (row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE) {
        return squares[row][col].get();
    }
    return nullptr;
}

void Board::drawBoardLabels(sf::RenderWindow& window, const sf::Font* font) {
    if (!font) return;
    
    sf::Text text;
    text.setFont(*font);
    text.setCharacterSize(18);
    text.setFillColor(sf::Color::Black);
    
    // Draw file labels (a-h) at the bottom
    for (int col = 0; col < BOARD_SIZE; col++) {
        text.setString(std::string(1, 'a' + col));
        text.setPosition(BOARD_OFFSET + col * SQUARE_SIZE + SQUARE_SIZE / 2 - 6, 
                        BOARD_OFFSET + BOARD_SIZE * SQUARE_SIZE + 10);
        window.draw(text);
    }
    
    // Draw rank labels (8-1) on the left side
    for (int row = 0; row < BOARD_SIZE; row++) {
        text.setString(std::string(1, '8' - row));
        text.setPosition(BOARD_OFFSET - 30, 
                        BOARD_OFFSET + row * SQUARE_SIZE + SQUARE_SIZE / 2 - 10);
        window.draw(text);
    }
}

void Board::setDraggedPiece(int row, int col, float screenX, float screenY) {
    draggedRow = row;
    draggedCol = col;
    draggedScreenX = screenX;
    draggedScreenY = screenY;
    isDraggingPiece = true;
}

void Board::clearDraggedPiece() {
    isDraggingPiece = false;
    draggedRow = -1;
    draggedCol = -1;
}

void Board::movePieceOnBoard(int fromRow, int fromCol, int toRow, int toCol) {
    std::cout << "[DEBUG] Moving piece from (" << fromRow << "," << fromCol << ") to (" 
              << toRow << "," << toCol << ")" << std::endl;
    
    if (fromRow >= 0 && fromRow < BOARD_SIZE && fromCol >= 0 && fromCol < BOARD_SIZE &&
        toRow >= 0 && toRow < BOARD_SIZE && toCol >= 0 && toCol < BOARD_SIZE) {
        
        // Check source has a piece
        if (!squares[fromRow][fromCol]) {
            std::cout << "[DEBUG] No piece at source!" << std::endl;
            return;
        }
        
        // Update piece position
        squares[fromRow][fromCol]->setPosition(toRow, toCol);
        
        // Move piece
        squares[toRow][toCol] = std::move(squares[fromRow][fromCol]);
        
        std::cout << "[DEBUG] Piece moved successfully" << std::endl;
    } else {
        std::cout << "[DEBUG] Invalid coordinates" << std::endl;
    }
}

void Board::drawMoveIndicators(sf::RenderWindow& window, const std::vector<std::pair<int, int>>& validMoves) {
    for (const auto& move : validMoves) {
        int row = move.first;
        int col = move.second;
        
        // Calculate center of the square
        float centerX = BOARD_OFFSET + col * SQUARE_SIZE + SQUARE_SIZE / 2.0f;  
        float centerY = BOARD_OFFSET + row * SQUARE_SIZE + SQUARE_SIZE / 2.0f;
        
        // Create a semi-transparent circle
        sf::CircleShape dot(8);  // 16px diameter dot
        dot.setFillColor(sf::Color(100, 100, 100, 150));  // Grey, semi-transparent
        dot.setPosition(centerX - 8, centerY - 8);  // Center on square
        
        window.draw(dot);
    }
}
