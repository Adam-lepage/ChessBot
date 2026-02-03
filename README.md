# Chess Game with SFML

A C++ chess game visualization using SFML, designed to be extended with game logic and an AI chess bot.

## Project Structure

```
ChessGame/
├── src/
│   ├── main.cpp          # Entry point
│   ├── Game.cpp          # Main game loop and window management
│   ├── Board.cpp         # Chess board rendering
│   └── Piece.cpp         # Individual piece rendering
├── include/
│   ├── Game.h            # Game class header
│   ├── Board.h           # Board class header
│   └── Piece.h           # Piece class header
├── assets/
│   └── pieces/           # PNG files for chess pieces (add your images here)
├── build/                # Build output directory
├── CMakeLists.txt        # CMake configuration
└── README.md             # This file
```

## Asset Setup

Piece images are located in `assets/pieces/` with the following files:

- `pawn-w.svg` / `pawn-b.svg`
- `rook-w.svg` / `rook-b.svg`
- `knight-w.svg` / `knight-b.svg`
- `bishop-w.svg` / `bishop-b.svg`
- `queen-w.svg` / `queen-b.svg`
- `king-w.svg` / `king-b.svg`

The pieces are automatically centered on each square. If textures fail to load, pieces will render as colored circles as a fallback.

## Class Overview

### Game
- Manages the main application window
- Handles the game loop (input, update, render)
- Window size: 1024x1024 (8 squares × 128 pixels)

### Board
- Renders the 8×8 chessboard with alternating light and dark squares
- Manages piece placement and initialization
- Piece layout follows standard chess starting position

### Piece
- Represents individual chess pieces
- Stores type (pawn, rook, knight, bishop, queen, king)
- Stores color (white/black)
- Handles texture loading and rendering
- Falls back to colored circles if textures unavailable

## Building

### Requirements
- GCC or Clang with C++17 support
- SFML 2.5+ (graphics, window, system modules)
- Make

### Building with Make

```bash
cd ChessGame
make
```

### Other Make Targets

```bash
make run       # Build and run the game
make clean     # Remove build artifacts
make rebuild   # Clean and build everything
```

## Running

After building, run the executable:
```bash
./build/bin/ChessGame
```

Or use the Make target:
```bash
make run
```

Press **ESC** or close the window to exit.

## Next Steps

This project is structured to easily add:
1. **Mouse Input Handling** - Click squares to select/move pieces
2. **Move Validation** - Implement chess move rules
3. **AI Bot** - Integrate minimax or other AI algorithms
4. **Game State** - Track captured pieces, check/checkmate, etc.

## Notes

- The board displays all pieces in their starting positions
- The window is 1024×1024 pixels (8×8 board, 128px per square)
- Piece textures are optional; colored circles display if PNGs aren't found
