#include "Piece.h"
#include <iostream>

Piece::Piece(PieceType type, PieceColor color, int row, int col)
    : type(type), color(color), row(row), col(col), textureLoaded(false) {
    // Try to load texture immediately
    std::string texturePath = "assets/pieces/" + getTextureFileName();
    loadTexture(texturePath);
}

Piece::~Piece() {
}

void Piece::draw(sf::RenderWindow& window, int squareSize, int boardOffsetX, int boardOffsetY) {
    if (textureLoaded) {
        // Calculate square center with offset
        float squareX = boardOffsetX + col * squareSize;
        float squareY = boardOffsetY + row * squareSize;
        float squareCenterX = squareX + squareSize / 2.0f;
        float squareCenterY = squareY + squareSize / 2.0f;
        
        // Get sprite dimensions for origin calculation
        sf::FloatRect bounds = sprite.getLocalBounds();
        
        // Center the sprite origin to its center, then position at square center
        sprite.setOrigin(bounds.width / 2.0f, bounds.height / 2.0f);
        sprite.setPosition(squareCenterX, squareCenterY);
        
        window.draw(sprite);
    } else {
        // Fallback: draw a simple colored circle if texture not loaded
        sf::CircleShape circle(squareSize / 2.5f);
        float centerX = boardOffsetX + col * squareSize + squareSize / 2.0f;
        float centerY = boardOffsetY + row * squareSize + squareSize / 2.0f;
        circle.setPosition(centerX - squareSize / 2.5f, centerY - squareSize / 2.5f);
        
        if (color == PieceColor::WHITE) {
            circle.setFillColor(sf::Color::White);
            circle.setOutlineThickness(2);
            circle.setOutlineColor(sf::Color::Black);
        } else {
            circle.setFillColor(sf::Color::Black);
        }
        
        window.draw(circle);
    }
}

void Piece::drawAtPosition(sf::RenderWindow& window, float x, float y) {
    if (textureLoaded) {
        // Get sprite dimensions for origin calculation
        sf::FloatRect bounds = sprite.getLocalBounds();
        
        // Center the sprite origin to its center
        sprite.setOrigin(bounds.width / 2.0f, bounds.height / 2.0f);
        sprite.setPosition(x, y);
        
        window.draw(sprite);
    }
}

void Piece::setPosition(int newRow, int newCol) {
    row = newRow;
    col = newCol;
}

bool Piece::loadTexture(const std::string& path) {
    if (texture.loadFromFile(path)) {
        sprite.setTexture(texture);
        
        // Scale texture to fit within square (80% of square size to add padding)
        float maxDimension = 128.0f * 0.8f;
        float textureWidth = texture.getSize().x;
        float textureHeight = texture.getSize().y;
        
        float scaleX = maxDimension / textureWidth;
        float scaleY = maxDimension / textureHeight;
        float scale = (scaleX < scaleY) ? scaleX : scaleY;
        
        sprite.setScale(scale, scale);
        
        textureLoaded = true;
        return true;
    } else {
        std::cerr << "Failed to load texture: " << path << std::endl;
        return false;
    }
}

std::string Piece::getTextureFileName() const {
    std::string filename;
    
    // Determine piece type and color suffix
    switch (type) {
        case PieceType::PAWN:
            filename = (color == PieceColor::WHITE) ? "pawn-w.png" : "pawn-b.png";
            break;
        case PieceType::ROOK:
            filename = (color == PieceColor::WHITE) ? "rook-w.png" : "rook-b.png";
            break;
        case PieceType::KNIGHT:
            filename = (color == PieceColor::WHITE) ? "knight-w.png" : "knight-b.png";
            break;
        case PieceType::BISHOP:
            filename = (color == PieceColor::WHITE) ? "bishop-w.png" : "bishop-b.png";
            break;
        case PieceType::QUEEN:
            filename = (color == PieceColor::WHITE) ? "queen-w.png" : "queen-b.png";
            break;
        case PieceType::KING:
            filename = (color == PieceColor::WHITE) ? "king-w.png" : "king-b.png";
            break;
        default:
            filename = "pawn-w.png";
    }
    
    return filename;
}
