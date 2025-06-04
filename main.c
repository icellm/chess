#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "chess.h"
#include "ai.h"
#include "ui.h"
#include "settings.h"

int main(int argc, char *argv[]) {
    // Seed random number generator
    srand((unsigned int)time(NULL));

    // Default options
    Settings settings = {
        .mode = MODE_HUMAN_VS_HUMAN,
        .difficulty = AI_MEDIUM,
        .theme = THEME_CLASSIC,
        .flipBoard = false,
    };
    strcpy(settings.pgnFile, "chess_save.pgn");
    const char *settingsFile = "settings.cfg";
    const char *loadFile = NULL;

    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--settings") == 0 && i + 1 < argc) {
            settingsFile = argv[++i];
        }
    }

    loadSettings(settingsFile, &settings);

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--ai") == 0 && i + 1 < argc) {
            settings.mode = MODE_HUMAN_VS_AI;
            i++;
            if (strcmp(argv[i], "easy") == 0) settings.difficulty = AI_EASY;
            else if (strcmp(argv[i], "medium") == 0) settings.difficulty = AI_MEDIUM;
            else if (strcmp(argv[i], "hard") == 0) settings.difficulty = AI_HARD;
            else if (strcmp(argv[i], "expert") == 0) settings.difficulty = AI_EXPERT;
        } else if (strcmp(argv[i], "--load") == 0 && i + 1 < argc) {
            loadFile = argv[++i];
        } else if (strcmp(argv[i], "--pgn") == 0 && i + 1 < argc) {
            strncpy(settings.pgnFile, argv[++i], sizeof(settings.pgnFile)-1);
            settings.pgnFile[sizeof(settings.pgnFile)-1] = '\0';
        } else if (strcmp(argv[i], "--flip") == 0) {
            settings.flipBoard = true;
        } else if (strcmp(argv[i], "--theme") == 0 && i + 1 < argc) {
            i++;
            if (strcmp(argv[i], "alt") == 0) settings.theme = THEME_ALT;
        }
    }

    // Initialize game state
    GameState gameState;
    initializeGame(&gameState);

    // Initialize game history
    GameHistory gameHistory;
    memset(&gameHistory, 0, sizeof(GameHistory));

    if (loadFile) {
        loadGame(&gameState, &gameHistory, loadFile);
    }

    // Initialize UI
    UIContext *ui = initUI(&gameState, &gameHistory, &settings);
    if (!ui) {
        fprintf(stderr, "Failed to initialize UI\n");
        return 1;
    }

    ui->gameMode = settings.mode;
    ui->aiDifficulty = settings.difficulty;
    ui->flipBoard = settings.flipBoard;
    ui->theme = settings.theme;
    applyTheme(ui);
    strncpy(ui->saveFile, settings.pgnFile, sizeof(ui->saveFile) - 1);

    // Run the game
    runUI(ui);

    // Save settings on exit
    saveSettings(settingsFile, &settings);

    // Clean up
    cleanupUI(ui);

    return 0;
}
