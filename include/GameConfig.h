#pragma once

#include <string>
#include <iostream>

// Game mode
enum class GameMode {
    PVP,    // Player vs Player (default)
    PVB,    // Player vs Bot
    BVB     // Bot vs Bot
};

// Configuration parsed from command-line arguments
struct GameConfig {
    GameMode mode = GameMode::PVP;
    bool debug = false;
    bool gui = true;          // true = show GUI, false = headless (console only)
    int playerColor = 0;      // 0 = white, 1 = black (for PVB mode)
    bool helpRequested = false; // Set when --help is used (exit code 0)
    bool modeSpecified = false; // Track if --mode was explicitly set
    int depth = -1;            // -1 = use bot default, otherwise override MAX_DEPTH
    int testBotGames = 0;      // 0 = normal play, >0 = run N games in test-bots mode
    bool silent = false;       // Suppress all cout output

    static void printUsage(const char* programName) {
        std::cout << "Usage: " << programName << " --mode <mode> [options]\n"
                  << "\nRequired:\n"
                  << "  --mode <mode>            Game mode (required)\n"
                  << "                             pvp  - Player vs Player\n"
                  << "                             pvb  - Player vs Bot\n"
                  << "                             bvb  - Bot vs Bot\n"
                  << "\nOptions:\n"
                  << "  -h, --help               Show this help message\n"
                  << "  -d, --debug              Enable debug output\n"
                  << "  --player-color <color>   Player color in pvb mode (default: white)\n"
                  << "                             white / black\n"
                  << "  --no-gui                 Disable GUI (auto-enabled for bvb)\n"
                  << "  --gui                    Force GUI on (even for bvb)\n"
                  << "  --depth <n>              Override bot search depth (default: bot's MAX_DEPTH)\n"
                  << "  --test-bots <n>          Run n games between the two bots (bvb mode)\n"
                  << "  --silent                 Suppress all game output (auto-enabled for test-bots)\n"
                  << "\nExamples:\n"
                  << "  " << programName << " --mode pvp                # Human vs Human with GUI\n"
                  << "  " << programName << " --mode pvb                # Play white vs random bot\n"
                  << "  " << programName << " --mode pvb --player-color black\n"
                  << "  " << programName << " --mode bvb --debug        # Bot vs Bot, headless + debug\n"
                  << "  " << programName << " --mode bvb --gui          # Bot vs Bot with GUI\n"
                  << "  " << programName << " --mode bvb --depth 5       # Bot vs Bot, depth 5\n"
                  << "  " << programName << " --mode bvb --test-bots 10  # 10 games, randomized colors\n"
                  << std::endl;
    }

    // Parse command-line arguments. Returns false if program should exit (e.g. --help).
    static bool parse(int argc, char* argv[], GameConfig& config) {
        for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];

            if (arg == "-h" || arg == "--help") {
                printUsage(argv[0]);
                config.helpRequested = true;
                return false;
            }
            else if (arg == "-d" || arg == "--debug") {
                config.debug = true;
            }
            else if (arg == "--no-gui") {
                config.gui = false;
            }
            else if (arg == "--gui") {
                config.gui = true;
            }
            else if (arg == "--mode") {
                if (i + 1 >= argc) {
                    std::cerr << "Error: --mode requires an argument (pvp, pvb, bvb)\n";
                    return false;
                }
                std::string mode = argv[++i];
                if (mode == "pvp") config.mode = GameMode::PVP;
                else if (mode == "pvb") config.mode = GameMode::PVB;
                else if (mode == "bvb") config.mode = GameMode::BVB;
                else {
                    std::cerr << "Error: Unknown mode '" << mode << "'. Use pvp, pvb, or bvb.\n";
                    return false;
                }
                config.modeSpecified = true;
            }
            else if (arg == "--player-color") {
                if (i + 1 >= argc) {
                    std::cerr << "Error: --player-color requires an argument (white, black)\n";
                    return false;
                }
                std::string color = argv[++i];
                if (color == "white" || color == "w") config.playerColor = 0;
                else if (color == "black" || color == "b") config.playerColor = 1;
                else {
                    std::cerr << "Error: Unknown color '" << color << "'. Use white or black.\n";
                    return false;
                }
            }
            else if (arg == "--depth") {
                if (i + 1 >= argc) {
                    std::cerr << "Error: --depth requires a positive integer\n";
                    return false;
                }
                config.depth = std::stoi(argv[++i]);
                if (config.depth < 1) {
                    std::cerr << "Error: --depth must be >= 1\n";
                    return false;
                }
            }
            else if (arg == "--silent") {
                config.silent = true;
            }
            else if (arg == "--test-bots") {
                if (i + 1 >= argc) {
                    std::cerr << "Error: --test-bots requires a positive integer\n";
                    return false;
                }
                config.testBotGames = std::stoi(argv[++i]);
                if (config.testBotGames < 1) {
                    std::cerr << "Error: --test-bots must be >= 1\n";
                    return false;
                }
            }

            else {
                std::cerr << "Error: Unknown argument '" << arg << "'\n";
                printUsage(argv[0]);
                return false;
            }
        }

        // Require explicit --mode argument
        if (!config.modeSpecified) {
            std::cerr << "Error: --mode is required\n\n";
            printUsage(argv[0]);
            return false;
        }

        return true;
    }
};
