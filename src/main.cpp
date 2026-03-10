#include "Game.h"
#include "GameConfig.h"
#include "RandomBot.h"
#include "Botv1.h"
#include "botv2.h"
#include "botv3.h"
#include <random>
#include <iostream>
#include <iomanip>
#include <atomic>
#include <omp.h>

int main(int argc, char* argv[]) {
    // Parse command-line arguments
    GameConfig config;
    if (!GameConfig::parse(argc, argv, config)) {
        return config.helpRequested ? 0 : 1;
    }

    // =================== Bot selection =====================
    Botv1 botv1;
    Botv2 botv2;
    Botv3 botv3;
    RandomBot randomBot;

    ChessBot* botA = &botv3;  // Bot A (used for pvb, bvb, test-bots)
    ChessBot* botB = &botv2;  // Bot B (opponent in bvb / test-bots)
    // ========================================================================

    // Apply --depth override if specified
    if (config.depth > 0) {
        botA->setMaxDepth(config.depth);
        botB->setMaxDepth(config.depth);
    }

    // Silence cout if --silent (for single-game mode)
    std::streambuf* origCoutBuf = nullptr;
    if (config.silent) {
        origCoutBuf = std::cout.rdbuf(nullptr);
    }

    // ---- test-bots mode: run N headless games with randomized colors ----
    if (config.testBotGames > 0) {
        config.mode = GameMode::BVB;
        config.gui = false;

        int botAWins = 0, botBWins = 0, draws = 0;
        std::atomic<int> completed{0};
        int totalGames = config.testBotGames;

#pragma omp parallel reduction(+:botAWins, botBWins, draws)
        {
            // Each thread gets its own bot instances (they have mutable state)
            Botv3 A;
            Botv2 B;
            RandomBot threadRandomBot;
            ChessBot* threadBotA = &A;
            ChessBot* threadBotB = &B;

            if (config.depth > 0) {
                threadBotA->setMaxDepth(config.depth);
                threadBotB->setMaxDepth(config.depth);
            }

            // Per-thread RNG seeded uniquely
            std::mt19937 rng(std::random_device{}() + omp_get_thread_num());

#pragma omp for schedule(dynamic)
            for (int g = 1; g <= totalGames; g++) {
                bool botAIsWhite = std::uniform_int_distribution<int>(0, 1)(rng) == 0;
                // bool botAIsWhite = false;

                Game game(config);
                if (botAIsWhite) {
                    game.setWhiteBot(threadBotA);
                    game.setBlackBot(threadBotB);
                } else {
                    game.setWhiteBot(threadBotB);
                    game.setBlackBot(threadBotA);
                }

                game.run();

                Game::GameResult result = game.getGameResult();
                if (result == Game::DRAW) {
                    draws++;
                } else if (result == Game::WHITE_WIN) {
                    if (botAIsWhite) botAWins++; else botBWins++;
                } else {
                    if (!botAIsWhite) botAWins++; else botBWins++;
                }

                int done = ++completed;
                std::cerr << "\rCompleted game " << done << "/" << totalGames << std::flush;
            }
        }

        std::cerr << std::endl; // newline after progress counter

        // Print summary
        std::cout << "\n========================================" << std::endl;
        std::cout << "        Test-Bots Results (" << config.testBotGames << " games)" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "  " << std::left << std::setw(12) << botA->getName()
                  << "  Wins: " << botAWins
                  << "  Losses: " << botBWins
                  << "  Draws: " << draws << std::endl;
        std::cout << "  " << std::left << std::setw(12) << botB->getName()
                  << "  Wins: " << botBWins
                  << "  Losses: " << botAWins
                  << "  Draws: " << draws << std::endl;
        std::cout << "========================================\n" << std::endl;

        return 0;
    }

    // ---- Normal single-game mode ----
    Game game(config);

    switch (config.mode) {
        case GameMode::PVP:
            break;

        case GameMode::PVB:
            if (config.playerColor == 0) {
                game.setBlackBot(botA);
            } else {
                game.setWhiteBot(botA);
            }
            break;

        case GameMode::BVB:
            game.setWhiteBot(botA);
            game.setBlackBot(botB);
            break;
    }

    game.run();
    return 0;
}
