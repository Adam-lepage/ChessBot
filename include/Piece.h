#pragma once

#include <SFML/Graphics.hpp>
#include <string>

enum class PieceType {
    PAWN,
    ROOK,
    KNIGHT,
    BISHOP,
    QUEEN,
    KING,
    NONE
};

enum class PieceColor {
    WHITE,
    BLACK
};

class Piece {
public:
    Piece(PieceType type, PieceColor color, int row, int col);
    ~Piece();
    
    void draw(sf::RenderWindow& window, int squareSize, int boardOffsetX = 0, int boardOffsetY = 0);
    void drawAtPosition(sf::RenderWindow& window, float x, float y);
    void setPosition(int row, int col);
    
    PieceType getType() const { return type; }
    PieceColor getColor() const { return color; }
    int getRow() const { return row; }
    int getCol() const { return col; }
    
    bool loadTexture(const std::string& path);

private:
    PieceType type;
    PieceColor color;
    int row;
    int col;
    
    sf::Sprite sprite;
    sf::Texture texture;
    bool textureLoaded;
    
    std::string getTextureFileName() const;
};
