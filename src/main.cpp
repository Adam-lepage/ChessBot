#include "Game.h"
#include "GameConfig.h"
#include "RandomBot.h"
#include "Botv1.h"
#include "Botv2.h"
#include <random>
#include <iostream>
#include <iomanip>

int main(int argc, char* argv[]) {
    // Parse command-line arguments
    GameConfig config;
    if (!GameConfig::parse(argc, argv, config)) {
        return config.helpRequested ? 0 : 1;
    }

    // Hardcoded bots
    Botv1 botv1;
    Botv2 botv2;
    RandomBot randomBot;

    // Apply --depth override if specified
    if (config.depth > 0) {
        botv1.setMaxDepth(config.depth);
        botv2.setMaxDepth(config.depth);
    }

    // ---- test-bots mode: run N headless games with randomized colors ----
    if (config.testBotGames > 0) {
        config.mode = GameMode::BVB;
        config.gui = false;

        std::mt19937 rng(std::random_device{}());

        int bot1Wins = 0, bot2Wins = 0, draws = 0;
        // bot1 = botv1, bot2 = botv2

        for (int g = 1; g <= config.testBotGames; g++) {
            // Randomize which bot plays white
            bool bot1IsWhite = std::uniform_int_distribution<int>(0, 1)(rng) == 0;

            Game game(config);
            if (bot1IsWhite) {
                game.setWhiteBot(&botv1);
                game.setBlackBot(&botv2);
            } else {
                game.setWhiteBot(&botv2);
                game.setBlackBot(&botv1);
            }

            std::cout << "=== Game " << g << "/" << config.testBotGames
                      << " | White: " << (bot1IsWhite ? botv1.getName() : botv2.getName())
                      << "  Black: " << (bot1IsWhite ? botv2.getName() : botv1.getName())
                      << " ===" << std::endl;

            game.run();

            Game::GameResult result = game.getGameResult();
            if (result == Game::DRAW) {
                draws++;
                std::cout << "Result: Draw\n" << std::endl;
            } else if (result == Game::WHITE_WIN) {
                if (bot1IsWhite) bot1Wins++; else bot2Wins++;
                std::cout << "Result: White (" << (bot1IsWhite ? botv1.getName() : botv2.getName()) << ") wins\n" << std::endl;
            } else {
                if (!bot1IsWhite) bot1Wins++; else bot2Wins++;
                std::cout << "Result: Black (" << (bot1IsWhite ? botv2.getName() : botv1.getName()) << ") wins\n" << std::endl;
            }
        }

        // Print summary
        std::cout << "\n========================================" << std::endl;
        std::cout << "        Test-Bots Results (" << config.testBotGames << " games)" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "  " << std::left << std::setw(12) << botv1.getName()
                  << "  Wins: " << bot1Wins
                  << "  Losses: " << bot2Wins
                  << "  Draws: " << draws << std::endl;
        std::cout << "  " << std::left << std::setw(12) << botv2.getName()
                  << "  Wins: " << bot2Wins
                  << "  Losses: " << bot1Wins
                  << "  Draws: " << draws << std::endl;
        std::cout << "========================================\n" << std::endl;

        return 0;
    }

    // ---- Normal single-game mode ----
    Game game(config);

    switch (config.mode) {
        case GameMode::PVP:
            // No bots — both sides are human
            break;

        case GameMode::PVB:
            // Player vs Bot: assign Botv2 to the non-player color
            if (config.playerColor == 0) {
                game.setBlackBot(&botv2);
            } else {
                game.setWhiteBot(&botv2);
            }
            break;

        case GameMode::BVB:
            // Bot vs Bot: Botv1 (white) vs Botv2 (black)
            game.setWhiteBot(&botv1);
            game.setBlackBot(&botv2);
            break;
    }

    game.run();
    return 0;
}
