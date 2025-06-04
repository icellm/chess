#ifndef ENGINE_H
#define ENGINE_H

#include "board.h"

// AI difficulty levels
typedef enum {
    AI_EASY = 2,     // 2 plies
    AI_MEDIUM = 3,   // 3 plies
    AI_HARD = 4,     // 4 plies
    AI_EXPERT = 5    // 5 plies
} AIDifficulty;

// Piece-Square tables used for position evaluation
extern const int PAWN_TABLE[64];
extern const int KNIGHT_TABLE[64];
extern const int BISHOP_TABLE[64];
extern const int ROOK_TABLE[64];
extern const int QUEEN_TABLE[64];
extern const int KING_TABLE_MIDDLE[64];
extern const int KING_TABLE_END[64];

// AI functions
Move getBestMove(GameState *state, AIDifficulty difficulty);
int evaluatePosition(const GameState *state);
int minimax(GameState *state, int depth, int alpha, int beta, bool maximizing, GameHistory *history);
int quiescenceSearch(GameState *state, int alpha, int beta, GameHistory *history);
bool isEndgame(const GameState *state);
int materialScore(const GameState *state);
int mobilityScore(const GameState *state);
int pawnStructureScore(const GameState *state);
int kingSafetyScore(const GameState *state);
int centerControlScore(const GameState *state);

#endif /* ENGINE_H */
