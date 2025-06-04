#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "chess.h"
#include "ai.h"
#include "ui.h"

int main(int argc, char *argv[]) {
    // Seed random number generator
    srand((unsigned int)time(NULL));
    
    // Initialize game state
    GameState gameState;
    initializeGame(&gameState);
    
    // Initialize game history
    GameHistory gameHistory;
    memset(&gameHistory, 0, sizeof(GameHistory));
    
    // Initialize UI
    UIContext *ui = initUI(&gameState, &gameHistory);
    if (!ui) {
        fprintf(stderr, "Failed to initialize UI\n");
        return 1;
    }
    
    // Run the game
    runUI(ui);
    
    // Clean up
    cleanupUI(ui);
    
    return 0;
}
