#include "Game.h"
#include "GameConfig.h"
#include "RandomBot.h"
#include "Botv1.h"

int main(int argc, char* argv[]) {
    // Parse command-line arguments
    GameConfig config;
    if (!GameConfig::parse(argc, argv, config)) {
        return config.helpRequested ? 0 : 1;
    }

    Game game(config);

    // Hardcoded bots
    Botv1 botv1;
    RandomBot randomBot;

    switch (config.mode) {
        case GameMode::PVP:
            // No bots — both sides are human
            break;

        case GameMode::PVB:
            // Player vs Bot: assign Botv1 to the non-player color
            if (config.playerColor == 0) {
                game.setBlackBot(&botv1);
            } else {
                game.setWhiteBot(&botv1);
            }
            break;

        case GameMode::BVB:
            // Bot vs Bot: Botv1 (white) vs RandomBot (black)
            game.setWhiteBot(&botv1);
            game.setBlackBot(&randomBot);
            break;
    }

    game.run();
    return 0;
}
