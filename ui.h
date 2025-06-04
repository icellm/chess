#ifndef UI_H
#define UI_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "chess.h"
#include "ai.h"

// UI constants
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define BOARD_SIZE_PX 480
#define SQUARE_SIZE (BOARD_SIZE_PX / 8)
#define BOARD_OFFSET_X 160
#define BOARD_OFFSET_Y 60

// UI colors
#define COLOR_LIGHT 0xEED6B9FF  // Light square
#define COLOR_DARK 0xB58863FF   // Dark square
#define COLOR_SELECTED 0xF7F76BFF // Selected square
#define COLOR_MOVE 0x706396FF   // Possible move
#define COLOR_BACKGROUND 0x282C34FF // Background
#define COLOR_TEXT 0xABB2BFFF   // Text color
#define COLOR_BUTTON 0x5C6370FF // Button color
#define COLOR_BUTTON_HOVER 0x767D89FF // Button hover color

// Game modes
typedef enum {
    MODE_HUMAN_VS_HUMAN,
    MODE_HUMAN_VS_AI
} GameMode;

// UI states
typedef enum {
    STATE_MENU,
    STATE_PLAYING,
    STATE_GAME_OVER
} UIState;

// Button structure
typedef struct {
    SDL_Rect rect;
    char text[64];
    bool hover;
} Button;

// UI context
typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    TTF_Font *font;
    TTF_Font *largeFont;
    
    // Game state
    GameState *gameState;
    GameHistory *gameHistory;
    GameMode gameMode;
    AIDifficulty aiDifficulty;
    UIState state;
    
    // Selection and move state
    int selectedRow;
    int selectedCol;
    bool pieceSelected;
    MoveList possibleMoves;
    
    // Animation
    bool animating;
    int animFrame;
    Move animMove;
    
    // Menu buttons
    Button btnNewGame;
    Button btnLoadGame;
    Button btnSaveGame;
    Button btnUndo;
    Button btnResign;
    Button btnMainMenu;
    Button btnHumanVsHuman;
    Button btnHumanVsAI;
    Button btnEasy;
    Button btnMedium;
    Button btnHard;
    Button btnExpert;
    
    // Message display
    char message[256];
    int messageTime;
} UIContext;

// UI initialization and cleanup
UIContext* initUI(GameState *state, GameHistory *history);
void cleanupUI(UIContext *ui);

// UI main loop
void runUI(UIContext *ui);

// UI event handling
void handleEvent(UIContext *ui, SDL_Event *event);

// UI rendering
void renderUI(UIContext *ui);
void renderBoard(UIContext *ui);
void renderPieces(UIContext *ui);
void renderMenu(UIContext *ui);
void renderGameOverScreen(UIContext *ui);
void renderButtons(UIContext *ui);
void renderMessage(UIContext *ui);
void renderCapturedPieces(UIContext *ui);

// Game logic
void selectSquare(UIContext *ui, int row, int col);
void resetSelection(UIContext *ui);
void makePlayerMove(UIContext *ui, int toRow, int toCol);
void makeAIMove(UIContext *ui);

// UI helpers
void setMessage(UIContext *ui, const char *format, ...);
Button createButton(int x, int y, int width, int height, const char *text);
bool isPointInRect(int x, int y, SDL_Rect *rect);
void drawButton(UIContext *ui, Button *button);

// Piece image loading and rendering
void loadPieceTextures(UIContext *ui);
void freePieceTextures(UIContext *ui);
void renderPieceAt(UIContext *ui, Piece piece, int x, int y);

#endif /* UI_H */