#include "Game.h"
#include "GameConfig.h"
#include "RandomBot.h"

int main(int argc, char* argv[]) {
    // Parse command-line arguments
    GameConfig config;
    if (!GameConfig::parse(argc, argv, config)) {
        return config.helpRequested ? 0 : 1;
    }

    Game game(config);

    // Hardcoded bots
    RandomBot randomBot1;
    RandomBot randomBot2;

    switch (config.mode) {
        case GameMode::PVP:
            // No bots — both sides are human
            break;

        case GameMode::PVB:
            // Player vs Bot: assign bot to the non-player color
            if (config.playerColor == 0) {
                game.setBlackBot(&randomBot1);
            } else {
                game.setWhiteBot(&randomBot1);
            }
            break;

        case GameMode::BVB:
            // Bot vs Bot: both are RandomBot
            game.setWhiteBot(&randomBot1);
            game.setBlackBot(&randomBot2);
            break;
    }

    game.run();
    return 0;
}
