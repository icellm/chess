#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "board.h"
#include "engine.h"
#include "gui.h"

int main(int argc, char *argv[]) {
    // Seed random number generator
    srand((unsigned int)time(NULL));

    // Default options
    GameMode mode = MODE_HUMAN_VS_HUMAN;
    AIDifficulty diff = AI_MEDIUM;
    const char *loadFile = NULL;
    const char *pgnFile = "chess_save.pgn";
    bool flipBoard = false;
    UITheme theme = THEME_CLASSIC;

    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--ai") == 0 && i + 1 < argc) {
            mode = MODE_HUMAN_VS_AI;
            i++;
            if (strcmp(argv[i], "easy") == 0) diff = AI_EASY;
            else if (strcmp(argv[i], "medium") == 0) diff = AI_MEDIUM;
            else if (strcmp(argv[i], "hard") == 0) diff = AI_HARD;
            else if (strcmp(argv[i], "expert") == 0) diff = AI_EXPERT;
        } else if (strcmp(argv[i], "--load") == 0 && i + 1 < argc) {
            loadFile = argv[++i];
        } else if (strcmp(argv[i], "--pgn") == 0 && i + 1 < argc) {
            pgnFile = argv[++i];
        } else if (strcmp(argv[i], "--flip") == 0) {
            flipBoard = true;
        } else if (strcmp(argv[i], "--theme") == 0 && i + 1 < argc) {
            i++;
            if (strcmp(argv[i], "alt") == 0) theme = THEME_ALT;
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
    UIContext *ui = initUI(&gameState, &gameHistory);
    if (!ui) {
        fprintf(stderr, "Failed to initialize UI\n");
        return 1;
    }

    ui->gameMode = mode;
    ui->aiDifficulty = diff;
    ui->flipBoard = flipBoard;
    ui->theme = theme;
    applyTheme(ui);
    strncpy(ui->saveFile, pgnFile, sizeof(ui->saveFile) - 1);

    // Run the game
    runUI(ui);

    // Clean up
    cleanupUI(ui);

    return 0;
}
