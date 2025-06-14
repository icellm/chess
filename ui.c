#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include "ui.h"

// Piece textures
SDL_Texture *pieceTextures[2][7]; // [color][piece type]

static inline int rowToY(UIContext *ui, int row) {
    return ui->flipBoard ? BOARD_OFFSET_Y + row * SQUARE_SIZE
                          : BOARD_OFFSET_Y + (7 - row) * SQUARE_SIZE;
}

static inline int colToX(UIContext *ui, int col) {
    return ui->flipBoard ? BOARD_OFFSET_X + (7 - col) * SQUARE_SIZE
                          : BOARD_OFFSET_X + col * SQUARE_SIZE;
}

static inline int yToRow(UIContext *ui, int y) {
    int r = (y - BOARD_OFFSET_Y) / SQUARE_SIZE;
    return ui->flipBoard ? r : 7 - r;
}

static inline int xToCol(UIContext *ui, int x) {
    int c = (x - BOARD_OFFSET_X) / SQUARE_SIZE;
    return ui->flipBoard ? 7 - c : c;
}

// Initialize UI and SDL components
UIContext* initUI(GameState *state, GameHistory *history) {
    UIContext *ui = (UIContext*)malloc(sizeof(UIContext));
    if (!ui) {
        fprintf(stderr, "Failed to allocate UI context\n");
        return NULL;
    }
    
    memset(ui, 0, sizeof(UIContext));
    ui->gameState = state;
    ui->gameHistory = history;
    ui->state = STATE_MENU;
    ui->gameMode = MODE_HUMAN_VS_HUMAN;
    ui->aiDifficulty = AI_MEDIUM;
    ui->theme = THEME_CLASSIC;
    ui->flipBoard = false;
    ui->lightColor = THEME_CLASSIC_LIGHT;
    ui->darkColor = THEME_CLASSIC_DARK;
    ui->backgroundColor = COLOR_BACKGROUND;
    ui->selectedRow = -1;
    ui->selectedCol = -1;
    ui->pieceSelected = false;
    ui->animating = false;
    ui->hasLastMove = false;
    strcpy(ui->saveFile, "chess_save.pgn");
    
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        free(ui);
        return NULL;
    }
    
    // Initialize SDL_ttf
    if (TTF_Init() != 0) {
        fprintf(stderr, "TTF_Init Error: %s\n", TTF_GetError());
        SDL_Quit();
        free(ui);
        return NULL;
    }
    
    // Initialize SDL_image
    if (IMG_Init(IMG_INIT_PNG) == 0) {
        fprintf(stderr, "IMG_Init Error: %s\n", IMG_GetError());
        TTF_Quit();
        SDL_Quit();
        free(ui);
        return NULL;
    }
    
    // Create window
    ui->window = SDL_CreateWindow("Chess", 
                                  SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                  WINDOW_WIDTH, WINDOW_HEIGHT, 
                                  SDL_WINDOW_SHOWN);
    if (!ui->window) {
        fprintf(stderr, "Window creation error: %s\n", SDL_GetError());
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        free(ui);
        return NULL;
    }
    
    // Create renderer
    ui->renderer = SDL_CreateRenderer(ui->window, -1, 
                                     SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ui->renderer) {
        fprintf(stderr, "Renderer creation error: %s\n", SDL_GetError());
        SDL_DestroyWindow(ui->window);
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        free(ui);
        return NULL;
    }
    
    // Load fonts
    ui->font = TTF_OpenFont("assets/fonts/DejaVuSans.ttf", 16);
    ui->largeFont = TTF_OpenFont("assets/fonts/DejaVuSans.ttf", 24);
    if (!ui->font || !ui->largeFont) {
        fprintf(stderr, "Font loading error: %s\n", TTF_GetError());
        fprintf(stderr, "Trying to load system font...\n");
        
        // Try system font paths as fallback
        #ifdef _WIN32
        ui->font = TTF_OpenFont("C:/Windows/Fonts/arial.ttf", 16);
        ui->largeFont = TTF_OpenFont("C:/Windows/Fonts/arial.ttf", 24);
        #elif __APPLE__
        ui->font = TTF_OpenFont("/Library/Fonts/Arial.ttf", 16);
        ui->largeFont = TTF_OpenFont("/Library/Fonts/Arial.ttf", 24);
        #else // Linux
        ui->font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 16);
        ui->largeFont = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 24);
        #endif
        
        if (!ui->font || !ui->largeFont) {
            fprintf(stderr, "Could not load any system font: %s\n", TTF_GetError());
            // Continue anyway with missing font, we'll handle this gracefully
        }
    }
    
    // Load piece textures
    loadPieceTextures(ui);
    
    // Create buttons
    ui->btnHumanVsHuman = createButton(WINDOW_WIDTH/2 - 100, 200, 200, 40, "Human vs Human");
    ui->btnHumanVsAI = createButton(WINDOW_WIDTH/2 - 100, 250, 200, 40, "Human vs AI");
    ui->btnEasy = createButton(WINDOW_WIDTH/2 - 200, 320, 90, 40, "Easy");
    ui->btnMedium = createButton(WINDOW_WIDTH/2 - 100, 320, 90, 40, "Medium");
    ui->btnHard = createButton(WINDOW_WIDTH/2, 320, 90, 40, "Hard");
    ui->btnExpert = createButton(WINDOW_WIDTH/2 + 100, 320, 90, 40, "Expert");
    
    ui->btnNewGame = createButton(20, 20, 120, 30, "New Game");
    ui->btnLoadGame = createButton(20, 60, 120, 30, "Load Game");
    ui->btnSaveGame = createButton(20, 100, 120, 30, "Save Game");
    ui->btnUndo = createButton(20, 140, 120, 30, "Undo Move");
    ui->btnResign = createButton(20, 180, 120, 30, "Resign");
    ui->btnMainMenu = createButton(20, 220, 120, 30, "Main Menu");
    ui->btnFlipBoard = createButton(20, 260, 120, 30, "Flip Board");
    ui->btnTheme = createButton(20, 300, 120, 30, "Change Theme");

    applyTheme(ui);

    return ui;
}

// Clean up UI resources
void cleanupUI(UIContext *ui) {
    if (!ui) return;
    
    freePieceTextures(ui); // Pass ui context
    
    if (ui->largeFont) TTF_CloseFont(ui->largeFont);
    if (ui->font) TTF_CloseFont(ui->font);
    if (ui->renderer) SDL_DestroyRenderer(ui->renderer);
    if (ui->window) SDL_DestroyWindow(ui->window);
    
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
    
    free(ui);
}

// Load piece textures from files or create them procedurally
void loadPieceTextures(UIContext *ui) {
    // Free existing textures first if any
    // This call might be problematic if pieceTextures is global and not tied to a specific ui instance yet.
    // However, to match the signature:
    freePieceTextures(ui); // Pass ui context

    // Initialize texture array
    memset(pieceTextures, 0, sizeof(pieceTextures));
    
    // Try to load textures from files
    const char *pieceNames[] = {"", "pawn", "knight", "bishop", "rook", "queen", "king"};
    const char *colorNames[] = {"white", "black"};
    
    bool texturesLoaded = true;
    char imagePath[256];
    
    for (int color = 0; color < 2; color++) {
        for (int piece = 1; piece <= 6; piece++) {
            sprintf(imagePath, "assets/pieces/%s_%s.png", colorNames[color], pieceNames[piece]);
            SDL_Surface *surface = IMG_Load(imagePath);
            
            if (surface) {
                pieceTextures[color][piece] = SDL_CreateTextureFromSurface(ui->renderer, surface);
                SDL_FreeSurface(surface);
                
                if (!pieceTextures[color][piece]) {
                    texturesLoaded = false;
                    break;
                }
            } else {
                texturesLoaded = false;
                break;
            }
        }
        if (!texturesLoaded) break;
    }
    
    // If loading from files failed, create procedural textures
    if (!texturesLoaded) {
        fprintf(stderr, "Failed to load piece textures, creating procedural ones\n");
        
        // Free any textures that were loaded
        freePieceTextures(ui);
        
        // Create procedural textures
        SDL_Color colors[2] = {
            {240, 240, 240, 255}, // White
            {60, 60, 60, 255}     // Black
        };
        
        const char *symbols[7] = {"", "P", "N", "B", "R", "Q", "K"};
        
        for (int color = 0; color < 2; color++) {
            for (int piece = 1; piece <= 6; piece++) {
                SDL_Surface *surface = SDL_CreateRGBSurface(0, SQUARE_SIZE, SQUARE_SIZE, 32, 
                                                          0, 0, 0, 0);
                if (!surface) continue;
                
                // Fill with transparent background
                SDL_FillRect(surface, NULL, SDL_MapRGBA(surface->format, 0, 0, 0, 0));
                
                // Render piece symbol
                if (ui->largeFont) {
                    SDL_Surface *textSurface = TTF_RenderText_Blended(ui->largeFont, 
                                                                     symbols[piece], 
                                                                     colors[color]);
                    if (textSurface) {
                        SDL_Rect destRect = {
                            SQUARE_SIZE/2 - textSurface->w/2,
                            SQUARE_SIZE/2 - textSurface->h/2,
                            textSurface->w,
                            textSurface->h
                        };
                        
                        SDL_BlitSurface(textSurface, NULL, surface, &destRect);
                        SDL_FreeSurface(textSurface);
                    }
                }
                
                pieceTextures[color][piece] = SDL_CreateTextureFromSurface(ui->renderer, surface);
                SDL_FreeSurface(surface);
            }
        }
    }
}

// Free piece textures
void freePieceTextures(UIContext *ui) { // Add UIContext *ui parameter
    for (int color = 0; color < 2; color++) {
        for (int piece = 1; piece <= 6; piece++) {
            if (pieceTextures[color][piece]) {
                SDL_DestroyTexture(pieceTextures[color][piece]);
                pieceTextures[color][piece] = NULL;
            }
        }
    }
}

// Render a piece at a specific position
void renderPieceAt(UIContext *ui, Piece piece, int x, int y) {
    if (piece == EMPTY) return;
    
    int pieceType = GET_PIECE_TYPE(piece);
    int color = GET_PIECE_COLOR(piece);
    
    if (pieceType < 1 || pieceType > 6) return;
    
    SDL_Texture *texture = pieceTextures[color][pieceType];
    if (!texture) return;
    
    SDL_Rect dstRect = {x, y, SQUARE_SIZE, SQUARE_SIZE};
    SDL_RenderCopy(ui->renderer, texture, NULL, &dstRect);
}

// Create a button structure
Button createButton(int x, int y, int width, int height, const char *text) {
    Button button;
    button.rect.x = x;
    button.rect.y = y;
    button.rect.w = width;
    button.rect.h = height;
    button.hover = false;
    strncpy(button.text, text, sizeof(button.text) - 1);
    button.text[sizeof(button.text) - 1] = '\0';
    
    return button;
}

// Check if a point is inside a rectangle
bool isPointInRect(int x, int y, SDL_Rect *rect) {
    return (x >= rect->x && x < rect->x + rect->w &&
            y >= rect->y && y < rect->y + rect->h);
}

// Draw a button
void drawButton(UIContext *ui, Button *button) {
    // Set button color based on hover state
    Uint32 color = button->hover ? COLOR_BUTTON_HOVER : COLOR_BUTTON;
    
    // Convert color to SDL format
    Uint8 r = (color >> 24) & 0xFF;
    Uint8 g = (color >> 16) & 0xFF;
    Uint8 b = (color >> 8) & 0xFF;
    Uint8 a = color & 0xFF;
    
    SDL_SetRenderDrawColor(ui->renderer, r, g, b, a);
    SDL_RenderFillRect(ui->renderer, &button->rect);
    
    // Draw button border
    SDL_SetRenderDrawColor(ui->renderer, r + 20, g + 20, b + 20, a);
    SDL_RenderDrawRect(ui->renderer, &button->rect);
    
    // Draw button text
    if (ui->font) {
        SDL_Color textColor = {255, 255, 255, 255};
        SDL_Surface *textSurface = TTF_RenderText_Blended(ui->font, button->text, textColor);
        
        if (textSurface) {
            SDL_Texture *textTexture = SDL_CreateTextureFromSurface(ui->renderer, textSurface);
            
            if (textTexture) {
                SDL_Rect textRect = {
                    button->rect.x + (button->rect.w - textSurface->w) / 2,
                    button->rect.y + (button->rect.h - textSurface->h) / 2,
                    textSurface->w,
                    textSurface->h
                };
                
                SDL_RenderCopy(ui->renderer, textTexture, NULL, &textRect);
                SDL_DestroyTexture(textTexture);
            }
            
            SDL_FreeSurface(textSurface);
        }
    }
}

// Set a message to display on the UI
void setMessage(UIContext *ui, const char *format, ...) {
    va_list args;
    va_start(args, format);
    vsnprintf(ui->message, sizeof(ui->message) - 1, format, args);
    va_end(args);
    
    ui->messageTime = 180; // Display for 3 seconds (60 fps * 3)
}

// Main UI loop
void runUI(UIContext *ui) {
    bool running = true;
    SDL_Event event;
    Uint32 lastTime = SDL_GetTicks();
    
    while (running) {
        // Handle events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else {
                handleEvent(ui, &event);
            }
        }
        
        // Update
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;
        
        // Handle AI move if it's AI's turn
        if (ui->state == STATE_PLAYING && 
            ui->gameMode == MODE_HUMAN_VS_AI && 
            ui->gameState->turn == BLACK && 
            !ui->animating && 
            !isCheckmate(ui->gameState) && 
            !isDraw(ui->gameState)) {
            makeAIMove(ui);
        }
        
        // Handle animations
        if (ui->animating) {
            ui->animFrame++;
            if (ui->animFrame >= 10) { // Animation completes in 10 frames
                ui->animating = false;
                
                // Check game state after animation completes
                if (isCheckmate(ui->gameState)) {
                    setMessage(ui, "Checkmate! %s wins!", 
                              ui->gameState->turn == WHITE ? "Black" : "White");
                    ui->state = STATE_GAME_OVER;
                }
                else if (isStalemate(ui->gameState)) {
                    setMessage(ui, "Stalemate! The game is a draw.");
                    ui->state = STATE_GAME_OVER;
                }
                else if (isDraw(ui->gameState)) {
                    setMessage(ui, "Draw!");
                    ui->state = STATE_GAME_OVER;
                }
                else if (isInCheck(ui->gameState, ui->gameState->turn)) {
                    setMessage(ui, "Check!");
                }
            }
        }
        
        // Update message timer
        if (ui->messageTime > 0) {
            ui->messageTime--;
        }
        
        // Render
        renderUI(ui);
        
        // Cap to ~60 FPS
        Uint32 frameTime = SDL_GetTicks() - currentTime;
        if (frameTime < 16) {
            SDL_Delay(16 - frameTime);
        }
    }
}

// Handle SDL events
void handleEvent(UIContext *ui, SDL_Event *event) {
    int mouseX, mouseY;
    
    switch (event->type) {
        case SDL_MOUSEMOTION:
            mouseX = event->motion.x;
            mouseY = event->motion.y;
            
            // Update button hover states
            if (ui->state == STATE_MENU) {
                ui->btnHumanVsHuman.hover = isPointInRect(mouseX, mouseY, &ui->btnHumanVsHuman.rect);
                ui->btnHumanVsAI.hover = isPointInRect(mouseX, mouseY, &ui->btnHumanVsAI.rect);
                ui->btnEasy.hover = isPointInRect(mouseX, mouseY, &ui->btnEasy.rect);
                ui->btnMedium.hover = isPointInRect(mouseX, mouseY, &ui->btnMedium.rect);
                ui->btnHard.hover = isPointInRect(mouseX, mouseY, &ui->btnHard.rect);
                ui->btnExpert.hover = isPointInRect(mouseX, mouseY, &ui->btnExpert.rect);
            } else {
                ui->btnNewGame.hover = isPointInRect(mouseX, mouseY, &ui->btnNewGame.rect);
                ui->btnLoadGame.hover = isPointInRect(mouseX, mouseY, &ui->btnLoadGame.rect);
                ui->btnSaveGame.hover = isPointInRect(mouseX, mouseY, &ui->btnSaveGame.rect);
                ui->btnUndo.hover = isPointInRect(mouseX, mouseY, &ui->btnUndo.rect);
                ui->btnResign.hover = isPointInRect(mouseX, mouseY, &ui->btnResign.rect);
                ui->btnMainMenu.hover = isPointInRect(mouseX, mouseY, &ui->btnMainMenu.rect);
                ui->btnFlipBoard.hover = isPointInRect(mouseX, mouseY, &ui->btnFlipBoard.rect);
                ui->btnTheme.hover = isPointInRect(mouseX, mouseY, &ui->btnTheme.rect);
            }
            break;
            
        case SDL_MOUSEBUTTONDOWN:
            if (event->button.button != SDL_BUTTON_LEFT) break;
            
            mouseX = event->button.x;
            mouseY = event->button.y;
            
            if (ui->state == STATE_MENU) {
                // Menu button handling
                if (isPointInRect(mouseX, mouseY, &ui->btnHumanVsHuman.rect)) {
                    ui->gameMode = MODE_HUMAN_VS_HUMAN;
                    resetGame(ui->gameState, ui->gameHistory);
                    ui->hasLastMove = false;
                    ui->state = STATE_PLAYING;
                    setMessage(ui, "New game: Human vs Human");
                }
                else if (isPointInRect(mouseX, mouseY, &ui->btnHumanVsAI.rect)) {
                    ui->gameMode = MODE_HUMAN_VS_AI;
                    resetGame(ui->gameState, ui->gameHistory);
                    ui->hasLastMove = false;
                    ui->state = STATE_PLAYING;
                    setMessage(ui, "New game: Human vs AI (%s)", 
                              ui->aiDifficulty == AI_EASY ? "Easy" :
                              ui->aiDifficulty == AI_MEDIUM ? "Medium" :
                              ui->aiDifficulty == AI_HARD ? "Hard" : "Expert");
                }
                else if (isPointInRect(mouseX, mouseY, &ui->btnEasy.rect)) {
                    ui->aiDifficulty = AI_EASY;
                }
                else if (isPointInRect(mouseX, mouseY, &ui->btnMedium.rect)) {
                    ui->aiDifficulty = AI_MEDIUM;
                }
                else if (isPointInRect(mouseX, mouseY, &ui->btnHard.rect)) {
                    ui->aiDifficulty = AI_HARD;
                }
                else if (isPointInRect(mouseX, mouseY, &ui->btnExpert.rect)) {
                    ui->aiDifficulty = AI_EXPERT;
                }
            }
            else if (ui->state == STATE_PLAYING || ui->state == STATE_GAME_OVER) {
                // Game UI button handling
                if (isPointInRect(mouseX, mouseY, &ui->btnNewGame.rect)) {
                    resetGame(ui->gameState, ui->gameHistory);
                    ui->hasLastMove = false;
                    ui->state = STATE_PLAYING;
                    setMessage(ui, "New game started");
                }
                else if (isPointInRect(mouseX, mouseY, &ui->btnLoadGame.rect)) {
                    if (loadGame(ui->gameState, ui->gameHistory, ui->saveFile)) {
                        ui->state = STATE_PLAYING;
                        setMessage(ui, "Game loaded successfully");
                    } else {
                        setMessage(ui, "Failed to load game");
                    }
                }
                else if (isPointInRect(mouseX, mouseY, &ui->btnSaveGame.rect)) {
                    if (saveGame(ui->gameState, ui->gameHistory, ui->saveFile)) {
                        setMessage(ui, "Game saved successfully");
                    } else {
                        setMessage(ui, "Failed to save game");
                    }
                }
                else if (isPointInRect(mouseX, mouseY, &ui->btnUndo.rect)) {
                    if (ui->gameMode == MODE_HUMAN_VS_HUMAN) {
                        undoMove(ui->gameState, ui->gameHistory);
                        ui->hasLastMove = false;
                        setMessage(ui, "Move undone");
                    } else if (ui->gameMode == MODE_HUMAN_VS_AI) {
                        // Undo both AI and human moves
                        undoMove(ui->gameState, ui->gameHistory);
                        undoMove(ui->gameState, ui->gameHistory);
                        ui->hasLastMove = false;
                        setMessage(ui, "Move undone");
                    }
                }
                else if (isPointInRect(mouseX, mouseY, &ui->btnResign.rect)) {
                    setMessage(ui, "%s resigns. %s wins!", 
                             ui->gameState->turn == WHITE ? "White" : "Black",
                             ui->gameState->turn == WHITE ? "Black" : "White");
                    ui->state = STATE_GAME_OVER;
                }
                else if (isPointInRect(mouseX, mouseY, &ui->btnMainMenu.rect)) {
                    ui->state = STATE_MENU;
                }
                else if (isPointInRect(mouseX, mouseY, &ui->btnFlipBoard.rect)) {
                    ui->flipBoard = !ui->flipBoard;
                }
                else if (isPointInRect(mouseX, mouseY, &ui->btnTheme.rect)) {
                    ui->theme = (ui->theme == THEME_CLASSIC) ? THEME_ALT : THEME_CLASSIC;
                    applyTheme(ui);
                }
                
                // Board interaction (only in playing state)
                else if (ui->state == STATE_PLAYING && 
                         mouseX >= BOARD_OFFSET_X && mouseX < BOARD_OFFSET_X + BOARD_SIZE_PX &&
                         mouseY >= BOARD_OFFSET_Y && mouseY < BOARD_OFFSET_Y + BOARD_SIZE_PX) {
                    
                    int col = xToCol(ui, mouseX);
                    int row = yToRow(ui, mouseY);
                    
                    if (!ui->animating) {
                        selectSquare(ui, row, col);
                    }
                }
            }
            break;
            
        case SDL_KEYDOWN:
            if (event->key.keysym.sym == SDLK_ESCAPE) {
                ui->state = STATE_MENU;
            }
            break;
    }
}

// Handle square selection and move logic
void selectSquare(UIContext *ui, int row, int col) {
    // Ignore if it's not the player's turn
    if ((ui->gameMode == MODE_HUMAN_VS_AI && ui->gameState->turn == BLACK) ||
        ui->gameState->turn != WHITE && ui->gameMode == MODE_HUMAN_VS_AI) {
        return;
    }
    
    Piece clickedPiece = getPiece(ui->gameState, row, col);
    
    // If no piece is selected, select this one if it's the player's
    if (!ui->pieceSelected) {
        if (clickedPiece != EMPTY && GET_PIECE_COLOR(clickedPiece) == ui->gameState->turn) {
            ui->selectedRow = row;
            ui->selectedCol = col;
            ui->pieceSelected = true;
            
            // Generate possible moves for this piece
            generateMoves(ui->gameState, &ui->possibleMoves);
        }
    }
    // If a piece is already selected
    else {
        // If clicking the same piece, deselect it
        if (row == ui->selectedRow && col == ui->selectedCol) {
            resetSelection(ui);
        }
        // If clicking another of the player's pieces, select that one instead
        else if (clickedPiece != EMPTY && GET_PIECE_COLOR(clickedPiece) == ui->gameState->turn) {
            ui->selectedRow = row;
            ui->selectedCol = col;
            
            // Generate possible moves for the newly selected piece
            generateMoves(ui->gameState, &ui->possibleMoves);
        }
        // Otherwise, try to move to the clicked square
        else {
            makePlayerMove(ui, row, col);
        }
    }
}

// Reset the current piece selection
void resetSelection(UIContext *ui) {
    ui->selectedRow = -1;
    ui->selectedCol = -1;
    ui->pieceSelected = false;
    ui->possibleMoves.count = 0;
}

// Attempt to make a player move
void makePlayerMove(UIContext *ui, int toRow, int toCol) {
    // Check if it's a valid move for the selected piece
    bool validMove = false;
    Move move = {ui->selectedRow, ui->selectedCol, toRow, toCol, 0};
    
    for (int i = 0; i < ui->possibleMoves.count; i++) {
        Move possibleMove = ui->possibleMoves.moves[i];
        if (possibleMove.fromRow == ui->selectedRow && 
            possibleMove.fromCol == ui->selectedCol &&
            possibleMove.toRow == toRow && 
            possibleMove.toCol == toCol) {
            
            validMove = true;
            move = possibleMove; // Use this move (might include promotion)
            break;
        }
    }
    
    // If this is a valid move for the selected piece
    if (validMove) {
        // Handle pawn promotion
        if (GET_PIECE_TYPE(getPiece(ui->gameState, ui->selectedRow, ui->selectedCol)) == PAWN &&
            ((ui->gameState->turn == WHITE && toRow == 7) || 
             (ui->gameState->turn == BLACK && toRow == 0))) {
            
            // Default to queen
            move.promotionPiece = QUEEN;
            
            // TODO: Add promotion piece selection dialog
        }
        
        // Start move animation
        ui->animating = true;
        ui->animFrame = 0;
        ui->animMove = move;
        
        // Execute the move
        makeMove(ui->gameState, move, ui->gameHistory);
        ui->lastMove = move;
        ui->hasLastMove = true;
        
        // Reset selection
        resetSelection(ui);
    }
}

// Make a move for the AI
void makeAIMove(UIContext *ui) {
    if (ui->animating) return;
    
    // Get AI move
    Move aiMove = getBestMove(ui->gameState, ui->aiDifficulty);
    
    // Invalid move check (no legal moves)
    if (aiMove.fromRow < 0) return;
    
    // Start move animation
    ui->animating = true;
    ui->animFrame = 0;
    ui->animMove = aiMove;
    
    // Execute the move
    makeMove(ui->gameState, aiMove, ui->gameHistory);
    ui->lastMove = aiMove;
    ui->hasLastMove = true;
}

// Render the entire UI
void renderUI(UIContext *ui) {
    // Set background color
    Uint8 bgR = (ui->backgroundColor >> 24) & 0xFF;
    Uint8 bgG = (ui->backgroundColor >> 16) & 0xFF;
    Uint8 bgB = (ui->backgroundColor >> 8) & 0xFF;
    Uint8 bgA = ui->backgroundColor & 0xFF;
    
    SDL_SetRenderDrawColor(ui->renderer, bgR, bgG, bgB, bgA);
    SDL_RenderClear(ui->renderer);
    
    if (ui->state == STATE_MENU) {
        renderMenu(ui);
    } else {
        renderBoard(ui);
        renderPieces(ui);
        renderButtons(ui);
        renderMoveHistory(ui);
        renderCapturedPieces(ui);
        
        if (ui->state == STATE_GAME_OVER) {
            renderGameOverScreen(ui);
        }
    }
    
    renderMessage(ui);
    
    SDL_RenderPresent(ui->renderer);
}

// Render the chess board
void renderBoard(UIContext *ui) {
    SDL_Rect square = {BOARD_OFFSET_X, BOARD_OFFSET_Y, SQUARE_SIZE, SQUARE_SIZE};
    
    for (int row = 0; row < BOARD_SIZE; row++) {
        for (int col = 0; col < BOARD_SIZE; col++) {
            // Set square position
            square.x = colToX(ui, col);
            square.y = rowToY(ui, row);
            
            // Determine square color (light or dark)
            Uint32 color;
            if ((row + col) % 2 == 0) {
                color = ui->lightColor;
            } else {
                color = ui->darkColor;
            }
            
            // Highlight last move squares
            if (ui->hasLastMove &&
                ((row == ui->lastMove.fromRow && col == ui->lastMove.fromCol) ||
                 (row == ui->lastMove.toRow && col == ui->lastMove.toCol))) {
                color = COLOR_LAST_MOVE;
            }

            // Highlight selected square
            if (ui->pieceSelected && row == ui->selectedRow && col == ui->selectedCol) {
                color = COLOR_SELECTED;
            }

            // Highlight possible moves
            if (ui->pieceSelected) {
                for (int i = 0; i < ui->possibleMoves.count; i++) {
                    if (ui->possibleMoves.moves[i].fromRow == ui->selectedRow &&
                        ui->possibleMoves.moves[i].fromCol == ui->selectedCol &&
                        ui->possibleMoves.moves[i].toRow == row &&
                        ui->possibleMoves.moves[i].toCol == col) {

                        color = COLOR_MOVE;
                        break;
                    }
                }
            }
            
            // Draw square
            Uint8 r = (color >> 24) & 0xFF;
            Uint8 g = (color >> 16) & 0xFF;
            Uint8 b = (color >> 8) & 0xFF;
            Uint8 a = color & 0xFF;
            
            SDL_SetRenderDrawColor(ui->renderer, r, g, b, a);
            SDL_RenderFillRect(ui->renderer, &square);
        }
    }
    
    // Draw board border
    SDL_Rect border = {
        BOARD_OFFSET_X - 2, 
        BOARD_OFFSET_Y - 2, 
        BOARD_SIZE_PX + 4, 
        BOARD_SIZE_PX + 4
    };
    SDL_SetRenderDrawColor(ui->renderer, 0, 0, 0, 255);
    SDL_RenderDrawRect(ui->renderer, &border);
    
    // Draw coordinates if font is available
    if (ui->font) {
        SDL_Color textColor = {200, 200, 200, 255};
        
        // File labels (a-h)
        for (int col = 0; col < BOARD_SIZE; col++) {
            char label[2] = {(char)('a' + col), '\0'};
            SDL_Surface *textSurface = TTF_RenderText_Blended(ui->font, label, textColor);
            
            if (textSurface) {
                SDL_Texture *textTexture = SDL_CreateTextureFromSurface(ui->renderer, textSurface);
                
                if (textTexture) {
                    SDL_Rect textRect = {
                        colToX(ui, col) + SQUARE_SIZE/2 - textSurface->w/2,
                        BOARD_OFFSET_Y + BOARD_SIZE_PX + 5,
                        textSurface->w,
                        textSurface->h
                    };
                    
                    SDL_RenderCopy(ui->renderer, textTexture, NULL, &textRect);
                    SDL_DestroyTexture(textTexture);
                }
                
                SDL_FreeSurface(textSurface);
            }
        }
        
        // Rank labels (1-8)
        for (int row = 0; row < BOARD_SIZE; row++) {
            int rank = ui->flipBoard ? 8 - row : row + 1;
            char label[2] = {(char)('0' + rank), '\0'};
            SDL_Surface *textSurface = TTF_RenderText_Blended(ui->font, label, textColor);
            
            if (textSurface) {
                SDL_Texture *textTexture = SDL_CreateTextureFromSurface(ui->renderer, textSurface);
                
                if (textTexture) {
                    SDL_Rect textRect = {
                        BOARD_OFFSET_X - textSurface->w - 5,
                        rowToY(ui, row) + SQUARE_SIZE/2 - textSurface->h/2,
                        textSurface->w,
                        textSurface->h
                    };
                    
                    SDL_RenderCopy(ui->renderer, textTexture, NULL, &textRect);
                    SDL_DestroyTexture(textTexture);
                }
                
                SDL_FreeSurface(textSurface);
            }
        }
    }
}

// Render chess pieces
void renderPieces(UIContext *ui) {
    for (int row = 0; row < BOARD_SIZE; row++) {
        for (int col = 0; col < BOARD_SIZE; col++) {
            // Skip the moving piece when animating
            if (ui->animating && row == ui->animMove.fromRow && col == ui->animMove.fromCol) {
                continue;
            }
            
            Piece piece = getPiece(ui->gameState, row, col);
            if (piece == EMPTY) continue;
            
            int x = colToX(ui, col);
            int y = rowToY(ui, row);
            
            renderPieceAt(ui, piece, x, y);
        }
    }
    
    // Render the animating piece
    if (ui->animating) {
        Piece piece = getPiece(ui->gameState, ui->animMove.toRow, ui->animMove.toCol);
        
        // Linear interpolation for position
        float progress = ui->animFrame / 10.0f;
        int startX = colToX(ui, ui->animMove.fromCol);
        int startY = rowToY(ui, ui->animMove.fromRow);
        int endX = colToX(ui, ui->animMove.toCol);
        int endY = rowToY(ui, ui->animMove.toRow);
        
        int x = startX + (int)((endX - startX) * progress);
        int y = startY + (int)((endY - startY) * progress);
        
        renderPieceAt(ui, piece, x, y);
    }
}

// Render main menu
void renderMenu(UIContext *ui) {
    // Draw title
    if (ui->largeFont) {
        SDL_Color titleColor = {255, 255, 255, 255};
        SDL_Surface *titleSurface = TTF_RenderText_Blended(ui->largeFont, "Chess Game", titleColor);
        
        if (titleSurface) {
            SDL_Texture *titleTexture = SDL_CreateTextureFromSurface(ui->renderer, titleSurface);
            
            if (titleTexture) {
                SDL_Rect titleRect = {
                    WINDOW_WIDTH/2 - titleSurface->w/2,
                    100,
                    titleSurface->w,
                    titleSurface->h
                };
                
                SDL_RenderCopy(ui->renderer, titleTexture, NULL, &titleRect);
                SDL_DestroyTexture(titleTexture);
            }
            
            SDL_FreeSurface(titleSurface);
        }
    }
    
    // Draw buttons
    drawButton(ui, &ui->btnHumanVsHuman);
    drawButton(ui, &ui->btnHumanVsAI);
    
    // Draw difficulty buttons (only show when Human vs AI is selected)
    if (ui->gameMode == MODE_HUMAN_VS_AI) {
        // Draw difficulty label
        if (ui->font) {
            SDL_Color textColor = {255, 255, 255, 255};
            SDL_Surface *textSurface = TTF_RenderText_Blended(ui->font, "AI Difficulty:", textColor);
            
            if (textSurface) {
                SDL_Texture *textTexture = SDL_CreateTextureFromSurface(ui->renderer, textSurface);
                
                if (textTexture) {
                    SDL_Rect textRect = {
                        WINDOW_WIDTH/2 - textSurface->w/2,
                        290,
                        textSurface->w,
                        textSurface->h
                    };
                    
                    SDL_RenderCopy(ui->renderer, textTexture, NULL, &textRect);
                    SDL_DestroyTexture(textTexture);
                }
                
                SDL_FreeSurface(textSurface);
            }
        }
        
        // Highlight the currently selected difficulty
        if (ui->aiDifficulty == AI_EASY) ui->btnEasy.hover = true;
        if (ui->aiDifficulty == AI_MEDIUM) ui->btnMedium.hover = true;
        if (ui->aiDifficulty == AI_HARD) ui->btnHard.hover = true;
        if (ui->aiDifficulty == AI_EXPERT) ui->btnExpert.hover = true;
        
        drawButton(ui, &ui->btnEasy);
        drawButton(ui, &ui->btnMedium);
        drawButton(ui, &ui->btnHard);
        drawButton(ui, &ui->btnExpert);
    }
}

// Render game over screen
void renderGameOverScreen(UIContext *ui) {
    // Semi-transparent overlay
    SDL_SetRenderDrawColor(ui->renderer, 0, 0, 0, 160);
    SDL_Rect overlay = {
        BOARD_OFFSET_X,
        BOARD_OFFSET_Y,
        BOARD_SIZE_PX,
        BOARD_SIZE_PX
    };
    SDL_RenderFillRect(ui->renderer, &overlay);
    
    // Game over text
    if (ui->largeFont) {
        SDL_Color textColor = {255, 255, 255, 255};
        SDL_Surface *textSurface = TTF_RenderText_Blended(ui->largeFont, "Game Over", textColor);
        
        if (textSurface) {
            SDL_Texture *textTexture = SDL_CreateTextureFromSurface(ui->renderer, textSurface);
            
            if (textTexture) {
                SDL_Rect textRect = {
                    BOARD_OFFSET_X + BOARD_SIZE_PX/2 - textSurface->w/2,
                    BOARD_OFFSET_Y + BOARD_SIZE_PX/2 - textSurface->h,
                    textSurface->w,
                    textSurface->h
                };
                
                SDL_RenderCopy(ui->renderer, textTexture, NULL, &textRect);
                SDL_DestroyTexture(textTexture);
            }
            
            SDL_FreeSurface(textSurface);
        }
    }
}

// Render UI buttons
void renderButtons(UIContext *ui) {
    drawButton(ui, &ui->btnNewGame);
    drawButton(ui, &ui->btnLoadGame);
    drawButton(ui, &ui->btnSaveGame);
    drawButton(ui, &ui->btnUndo);
    drawButton(ui, &ui->btnResign);
    drawButton(ui, &ui->btnMainMenu);
    drawButton(ui, &ui->btnFlipBoard);
    drawButton(ui, &ui->btnTheme);
    
    // Render current player's turn
    if (ui->font) {
        char turnText[32];
        sprintf(turnText, "Turn: %s", ui->gameState->turn == WHITE ? "White" : "Black");
        
        SDL_Color textColor = {255, 255, 255, 255};
        SDL_Surface *textSurface = TTF_RenderText_Blended(ui->font, turnText, textColor);
        
        if (textSurface) {
            SDL_Texture *textTexture = SDL_CreateTextureFromSurface(ui->renderer, textSurface);
            
            if (textTexture) {
                SDL_Rect textRect = {
                    BOARD_OFFSET_X,
                    BOARD_OFFSET_Y + BOARD_SIZE_PX + 40,
                    textSurface->w,
                    textSurface->h
                };
                
                SDL_RenderCopy(ui->renderer, textTexture, NULL, &textRect);
                SDL_DestroyTexture(textTexture);
            }
            
            SDL_FreeSurface(textSurface);
        }
    }
}

// Render message if there is one
void renderMessage(UIContext *ui) {
    if (ui->messageTime > 0 && ui->font) {
        SDL_Color textColor = {255, 255, 100, 255};
        SDL_Surface *textSurface = TTF_RenderText_Blended(ui->font, ui->message, textColor);
        
        if (textSurface) {
            SDL_Texture *textTexture = SDL_CreateTextureFromSurface(ui->renderer, textSurface);
            
            if (textTexture) {
                SDL_Rect textRect = {
                    (WINDOW_WIDTH - textSurface->w) / 2,
                    WINDOW_HEIGHT - textSurface->h - 20,
                    textSurface->w,
                    textSurface->h
                };
                
                SDL_RenderCopy(ui->renderer, textTexture, NULL, &textRect);
                SDL_DestroyTexture(textTexture);
            }
            
            SDL_FreeSurface(textSurface);
        }
    }
}

// Render captured pieces
void renderCapturedPieces(UIContext *ui) {
    // Calculate captured pieces for both sides
    int capturedWhite[6] = {0}; // Count of white pieces captured by black
    int capturedBlack[6] = {0}; // Count of black pieces captured by white
    
    // Piece values for material advantage calculation
    const int pieceValues[7] = {0, 1, 3, 3, 5, 9, 0}; // Empty, P, N, B, R, Q, K
    
    // Count pieces on the board
    int piecesOnBoard[2][7] = {0}; // [color][piece type]
    
    for (int row = 0; row < BOARD_SIZE; row++) {
        for (int col = 0; col < BOARD_SIZE; col++) {
            Piece piece = getPiece(ui->gameState, row, col);
            if (piece != EMPTY) {
                int pieceType = GET_PIECE_TYPE(piece);
                int color = GET_PIECE_COLOR(piece);
                piecesOnBoard[color][pieceType]++;
            }
        }
    }
    
    // Initial piece counts
    const int initialPieces[7] = {0, 8, 2, 2, 2, 1, 1}; // Empty, P, N, B, R, Q, K
    
    // Calculate captured pieces
    for (int piece = 1; piece <= 6; piece++) {
        capturedWhite[piece-1] = initialPieces[piece] - piecesOnBoard[WHITE][piece];
        capturedBlack[piece-1] = initialPieces[piece] - piecesOnBoard[BLACK][piece];
    }
    
    // Calculate material advantage
    int whiteMaterial = 0;
    int blackMaterial = 0;
    
    for (int row = 0; row < BOARD_SIZE; row++) {
        for (int col = 0; col < BOARD_SIZE; col++) {
            Piece piece = getPiece(ui->gameState, row, col);
            if (piece != EMPTY) {
                int pieceType = GET_PIECE_TYPE(piece);
                int color = GET_PIECE_COLOR(piece);
                
                if (color == WHITE) {
                    whiteMaterial += pieceValues[pieceType];
                } else {
                    blackMaterial += pieceValues[pieceType];
                }
            }
        }
    }
    
    int advantage = whiteMaterial - blackMaterial;
    
    // Render material advantage
    if (ui->font && advantage != 0) {
        char advantageText[32];
        sprintf(advantageText, "Advantage: %s%d", 
               advantage > 0 ? "+" : "",
               advantage);
        
        SDL_Color textColor = {255, 255, 255, 255};
        SDL_Surface *textSurface = TTF_RenderText_Blended(ui->font, advantageText, textColor);
        
        if (textSurface) {
            SDL_Texture *textTexture = SDL_CreateTextureFromSurface(ui->renderer, textSurface);
            
            if (textTexture) {
                SDL_Rect textRect = {
                    20,
                    270,
                    textSurface->w,
                    textSurface->h
                };
                
                SDL_RenderCopy(ui->renderer, textTexture, NULL, &textRect);
                SDL_DestroyTexture(textTexture);
            }
            
            SDL_FreeSurface(textSurface);
        }
    }
    
    // Display captured pieces (simplified representation)
    // For white pieces captured by black
    int offsetY = 320;
    if (ui->font) {
        SDL_Color textColor = {200, 200, 200, 255};
        SDL_Surface *labelSurface = TTF_RenderText_Blended(ui->font, "Captured White:", textColor);
        
        if (labelSurface) {
            SDL_Texture *labelTexture = SDL_CreateTextureFromSurface(ui->renderer, labelSurface);
            
            if (labelTexture) {
                SDL_Rect labelRect = {
                    20,
                    offsetY,
                    labelSurface->w,
                    labelSurface->h
                };
                
                SDL_RenderCopy(ui->renderer, labelTexture, NULL, &labelRect);
                SDL_DestroyTexture(labelTexture);
            }
            
            SDL_FreeSurface(labelSurface);
        }
    }
    
    // Display captured white pieces
    char capturedText[128] = "";
    const char *symbols[] = {"", "P", "N", "B", "R", "Q"};
    
    for (int piece = 0; piece < 6; piece++) {
        for (int count = 0; count < capturedWhite[piece]; count++) {
            strcat(capturedText, symbols[piece]);
            strcat(capturedText, " ");
        }
    }
    
    if (ui->font && strlen(capturedText) > 0) {
        SDL_Color textColor = {255, 255, 255, 255};
        SDL_Surface *pieceSurface = TTF_RenderText_Blended(ui->font, capturedText, textColor);
        
        if (pieceSurface) {
            SDL_Texture *pieceTexture = SDL_CreateTextureFromSurface(ui->renderer, pieceSurface);
            
            if (pieceTexture) {
                SDL_Rect pieceRect = {
                    20,
                    offsetY + 25,
                    pieceSurface->w,
                    pieceSurface->h
                };
                
                SDL_RenderCopy(ui->renderer, pieceTexture, NULL, &pieceRect);
                SDL_DestroyTexture(pieceTexture);
            }
            
            SDL_FreeSurface(pieceSurface);
        }
    }
    
    // For black pieces captured by white
    offsetY = 380;
    if (ui->font) {
        SDL_Color textColor = {200, 200, 200, 255};
        SDL_Surface *labelSurface = TTF_RenderText_Blended(ui->font, "Captured Black:", textColor);
        
        if (labelSurface) {
            SDL_Texture *labelTexture = SDL_CreateTextureFromSurface(ui->renderer, labelSurface);
            
            if (labelTexture) {
                SDL_Rect labelRect = {
                    20,
                    offsetY,
                    labelSurface->w,
                    labelSurface->h
                };
                
                SDL_RenderCopy(ui->renderer, labelTexture, NULL, &labelRect);
                SDL_DestroyTexture(labelTexture);
            }
            
            SDL_FreeSurface(labelSurface);
        }
    }
    
    // Display captured black pieces
    capturedText[0] = '\0';
    for (int piece = 0; piece < 6; piece++) {
        for (int count = 0; count < capturedBlack[piece]; count++) {
            strcat(capturedText, symbols[piece]);
            strcat(capturedText, " ");
        }
    }
    
    if (ui->font && strlen(capturedText) > 0) {
        SDL_Color textColor = {80, 80, 80, 255};
        SDL_Surface *pieceSurface = TTF_RenderText_Blended(ui->font, capturedText, textColor);
        
        if (pieceSurface) {
            SDL_Texture *pieceTexture = SDL_CreateTextureFromSurface(ui->renderer, pieceSurface);
            
            if (pieceTexture) {
                SDL_Rect pieceRect = {
                    20,
                    offsetY + 25,
                    pieceSurface->w,
                    pieceSurface->h
                };
                
                SDL_RenderCopy(ui->renderer, pieceTexture, NULL, &pieceRect);
                SDL_DestroyTexture(pieceTexture);
            }
            
            SDL_FreeSurface(pieceSurface);
        }
    }
}

// Render list of recent moves on the right side of the board
void renderMoveHistory(UIContext *ui) {
    if (!ui->font) return;

    char pgnCopy[8192];
    strncpy(pgnCopy, ui->gameHistory->pgn, sizeof(pgnCopy)-1);
    pgnCopy[sizeof(pgnCopy)-1] = '\0';

    const char *tokens[256];
    int count = 0;

    char *token = strtok(pgnCopy, " \n\r");
    while (token && count < 256) {
        if (strchr(token, '.')) {
            token = strtok(NULL, " \n\r");
            continue;
        }
        tokens[count++] = token;
        token = strtok(NULL, " \n\r");
    }

    int start = (count > 16) ? count - 16 : 0; // show last 8 moves
    int y = BOARD_OFFSET_Y;

    for (int i = start; i < count; i += 2) {
        char line[32];
        int moveNum = i/2 + 1;
        if (i + 1 < count) {
            snprintf(line, sizeof(line), "%d. %s %s", moveNum, tokens[i], tokens[i+1]);
        } else {
            snprintf(line, sizeof(line), "%d. %s", moveNum, tokens[i]);
        }

        SDL_Color textColor = {200, 200, 200, 255};
        SDL_Surface *surf = TTF_RenderText_Blended(ui->font, line, textColor);
        if (surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(ui->renderer, surf);
            if (tex) {
                SDL_Rect rect = {BOARD_OFFSET_X + BOARD_SIZE_PX + 10, y, surf->w, surf->h};
                SDL_RenderCopy(ui->renderer, tex, NULL, &rect);
                SDL_DestroyTexture(tex);
            }
            SDL_FreeSurface(surf);
        }
        y += 18;
    }
}

// Apply color theme to UI
void applyTheme(UIContext *ui) {
    if (ui->theme == THEME_ALT) {
        ui->lightColor = THEME_ALT_LIGHT;
        ui->darkColor = THEME_ALT_DARK;
        ui->backgroundColor = 0x1E1E1EFF;
    } else {
        ui->lightColor = THEME_CLASSIC_LIGHT;
        ui->darkColor = THEME_CLASSIC_DARK;
        ui->backgroundColor = COLOR_BACKGROUND;
    }
}
