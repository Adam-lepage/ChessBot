# Chess Game with SFML

A C++ chess game visualization using SFML, designed to be extended with game logic and an AI chess bot.

### Building with Make

```bash
cd ChessGame
make
```

## Running

Usage: ./ChessGame --mode <mode> [options]

Required:
  --mode <mode>            Game mode (required)
                             pvp  - Player vs Player
                             pvb  - Player vs Bot
                             bvb  - Bot vs Bot

Options:
  -h, --help               Show this help message
  -d, --debug              Enable debug output
  --player-color <color>   Player color in pvb mode (default: white)
                             white / black
  --no-gui                 Disable GUI (auto-enabled for bvb)
  --gui                    Force GUI on (even for bvb)

