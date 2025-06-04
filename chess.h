#ifndef CHESS_H
#define CHESS_H

#include <stdbool.h>
#include <stdint.h>

#define BOARD_SIZE 8
#define WHITE 0
#define BLACK 1
#define EMPTY 0
#define PAWN 1
#define KNIGHT 2
#define BISHOP 3
#define ROOK 4
#define QUEEN 5
#define KING 6

/* Piece representation
 * Bit 0-2: Piece type (0=empty, 1=pawn, 2=knight, 3=bishop, 4=rook, 5=queen, 6=king)
 * Bit 3: Color (0=white, 1=black)
 * Bit 4: Has moved flag (for castling and pawn double move)
 */
typedef uint8_t Piece;
typedef uint8_t Color;

typedef struct {
    Piece board[BOARD_SIZE][BOARD_SIZE];
    Color turn;
    bool castlingRights[2][2]; // [color][side] where side 0=queenside, 1=kingside
    int enPassantCol;          // Column of pawn that moved two squares, -1 if none
    int halfMoveClock;         // Moves since last capture or pawn advance (for 50-move rule)
    int fullMoveNumber;        // Incremented after Black's move
    int capturedPieces[2][6];  // Count of captured pieces [color][piece_type-1]
} GameState;

typedef struct {
    int fromRow;
    int fromCol;
    int toRow;
    int toCol;
    int promotionPiece; // 0 if not a promotion
} Move;

typedef struct {
    Move moves[256]; // Max possible legal moves in a position
    int count;
} MoveList;

typedef struct {
    GameState state;
    Move lastMove;
    Piece capturedPiece;
    bool wasEnPassant;
    bool wasCastling;
    bool wasPromotion;
    int oldEnPassantCol;
    int oldHalfMoveClock;
    bool oldCastlingRights[2][2];
} MoveHistory;

typedef struct {
    MoveHistory history[1024]; // Stack for undo/redo
    int historyCount;
    int historyIndex;
    char pgn[8192];           // Game notation in PGN format
    int pgnLength;
} GameHistory;

// Board and game initialization
void initializeGame(GameState *state);
void resetGame(GameState *state, GameHistory *history);

// Move generation and validation
bool isValidMove(const GameState *state, Move move);
void generateMoves(const GameState *state, MoveList *moves);
bool isInCheck(const GameState *state, Color color);
bool isCheckmate(const GameState *state);
bool isStalemate(const GameState *state);
bool isDraw(const GameState *state);
bool isThreefoldRepetition(const GameHistory *history);
bool isFiftyMoveDraw(const GameState *state);
bool isInsufficientMaterial(const GameState *state);

// Move execution
bool makeMove(GameState *state, Move move, GameHistory *history);
void undoMove(GameState *state, GameHistory *history);
void redoMove(GameState *state, GameHistory *history);

// Utility functions
Piece getPiece(const GameState *state, int row, int col);
void setPiece(GameState *state, int row, int col, Piece piece);
bool isSquareAttacked(const GameState *state, int row, int col, Color attackingColor);
void algebraicToMove(const char *algebraic, Move *move);
void moveToAlgebraic(const GameState *state, Move move, char *algebraic);
void addMoveToPGN(GameState *state, Move move, GameHistory *history);

// Save and load game
bool saveGame(const GameState *state, const GameHistory *history, const char *filename);
bool loadGame(GameState *state, GameHistory *history, const char *filename);

// Piece manipulation
#define CREATE_PIECE(type, color) ((Piece)((type) | ((color) << 3)))
#define GET_PIECE_TYPE(piece) ((piece) & 0x07)
#define GET_PIECE_COLOR(piece) (((piece) >> 3) & 0x01)
#define SET_PIECE_MOVED(piece) ((piece) | 0x10)
#define HAS_PIECE_MOVED(piece) (((piece) >> 4) & 0x01)

// Debug functions
void printBoard(const GameState *state);
char getPieceChar(Piece piece);

#endif /* CHESS_H */