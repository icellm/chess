#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "board.h"

// Initialize the chess board with pieces in their starting positions
void initializeGame(GameState *state) {
    memset(state, 0, sizeof(GameState));
    
    // Set up initial positions for pawns
    for (int col = 0; col < BOARD_SIZE; col++) {
        setPiece(state, 1, col, CREATE_PIECE(PAWN, WHITE));
        setPiece(state, 6, col, CREATE_PIECE(PAWN, BLACK));
    }
    
    // Set up initial positions for other pieces
    setPiece(state, 0, 0, CREATE_PIECE(ROOK, WHITE));
    setPiece(state, 0, 1, CREATE_PIECE(KNIGHT, WHITE));
    setPiece(state, 0, 2, CREATE_PIECE(BISHOP, WHITE));
    setPiece(state, 0, 3, CREATE_PIECE(QUEEN, WHITE));
    setPiece(state, 0, 4, CREATE_PIECE(KING, WHITE));
    setPiece(state, 0, 5, CREATE_PIECE(BISHOP, WHITE));
    setPiece(state, 0, 6, CREATE_PIECE(KNIGHT, WHITE));
    setPiece(state, 0, 7, CREATE_PIECE(ROOK, WHITE));
    
    setPiece(state, 7, 0, CREATE_PIECE(ROOK, BLACK));
    setPiece(state, 7, 1, CREATE_PIECE(KNIGHT, BLACK));
    setPiece(state, 7, 2, CREATE_PIECE(BISHOP, BLACK));
    setPiece(state, 7, 3, CREATE_PIECE(QUEEN, BLACK));
    setPiece(state, 7, 4, CREATE_PIECE(KING, BLACK));
    setPiece(state, 7, 5, CREATE_PIECE(BISHOP, BLACK));
    setPiece(state, 7, 6, CREATE_PIECE(KNIGHT, BLACK));
    setPiece(state, 7, 7, CREATE_PIECE(ROOK, BLACK));
    
    // Initialize game state
    state->turn = WHITE;
    state->enPassantCol = -1;
    state->halfMoveClock = 0;
    state->fullMoveNumber = 1;
    
    // Initialize castling rights
    state->castlingRights[WHITE][0] = true; // White queenside
    state->castlingRights[WHITE][1] = true; // White kingside
    state->castlingRights[BLACK][0] = true; // Black queenside
    state->castlingRights[BLACK][1] = true; // Black kingside
}

void resetGame(GameState *state, GameHistory *history) {
    initializeGame(state);
    memset(history, 0, sizeof(GameHistory));
}

// Get piece at a specific position
Piece getPiece(const GameState *state, int row, int col) {
    if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE) {
        return 0;
    }
    return state->board[row][col];
}

// Set piece at a specific position
void setPiece(GameState *state, int row, int col, Piece piece) {
    if (row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE) {
        state->board[row][col] = piece;
    }
}

// Check if a square is attacked by a specific color
bool isSquareAttacked(const GameState *state, int row, int col, Color attackingColor) {
    // Check for attacking pawns
    int pawnRow = (attackingColor == WHITE) ? row - 1 : row + 1;
    if (pawnRow >= 0 && pawnRow < BOARD_SIZE) {
        // Left diagonal
        if (col - 1 >= 0) {
            Piece piece = getPiece(state, pawnRow, col - 1);
            if (GET_PIECE_TYPE(piece) == PAWN && GET_PIECE_COLOR(piece) == attackingColor) {
                return true;
            }
        }
        // Right diagonal
        if (col + 1 < BOARD_SIZE) {
            Piece piece = getPiece(state, pawnRow, col + 1);
            if (GET_PIECE_TYPE(piece) == PAWN && GET_PIECE_COLOR(piece) == attackingColor) {
                return true;
            }
        }
    }
    
    // Knight moves
    const int knightMoves[8][2] = {
        {-2, -1}, {-2, 1}, {-1, -2}, {-1, 2},
        {1, -2}, {1, 2}, {2, -1}, {2, 1}
    };
    
    for (int i = 0; i < 8; i++) {
        int newRow = row + knightMoves[i][0];
        int newCol = col + knightMoves[i][1];
        
        if (newRow >= 0 && newRow < BOARD_SIZE && newCol >= 0 && newCol < BOARD_SIZE) {
            Piece piece = getPiece(state, newRow, newCol);
            if (GET_PIECE_TYPE(piece) == KNIGHT && GET_PIECE_COLOR(piece) == attackingColor) {
                return true;
            }
        }
    }
    
    // Directions for sliding pieces (rook, bishop, queen)
    const int directions[8][2] = {
        {-1, 0}, {1, 0}, {0, -1}, {0, 1}, // Rook/Queen
        {-1, -1}, {-1, 1}, {1, -1}, {1, 1} // Bishop/Queen
    };
    
    for (int i = 0; i < 8; i++) {
        int dr = directions[i][0];
        int dc = directions[i][1];
        
        int newRow = row + dr;
        int newCol = col + dc;
        
        while (newRow >= 0 && newRow < BOARD_SIZE && newCol >= 0 && newCol < BOARD_SIZE) {
            Piece piece = getPiece(state, newRow, newCol);
            
            if (piece != EMPTY) {
                if (GET_PIECE_COLOR(piece) == attackingColor) {
                    int pieceType = GET_PIECE_TYPE(piece);
                    
                    if (pieceType == QUEEN || 
                        (pieceType == ROOK && i < 4) ||
                        (pieceType == BISHOP && i >= 4)) {
                        return true;
                    }
                }
                break; // Blocked by a piece
            }
            
            newRow += dr;
            newCol += dc;
        }
    }
    
    // King attacks
    const int kingMoves[8][2] = {
        {-1, -1}, {-1, 0}, {-1, 1}, {0, -1},
        {0, 1}, {1, -1}, {1, 0}, {1, 1}
    };
    
    for (int i = 0; i < 8; i++) {
        int newRow = row + kingMoves[i][0];
        int newCol = col + kingMoves[i][1];
        
        if (newRow >= 0 && newRow < BOARD_SIZE && newCol >= 0 && newCol < BOARD_SIZE) {
            Piece piece = getPiece(state, newRow, newCol);
            if (GET_PIECE_TYPE(piece) == KING && GET_PIECE_COLOR(piece) == attackingColor) {
                return true;
            }
        }
    }
    
    return false;
}

// Check if a king is in check
bool isInCheck(const GameState *state, Color color) {
    // Find the king position
    int kingRow = -1, kingCol = -1;
    
    for (int row = 0; row < BOARD_SIZE; row++) {
        for (int col = 0; col < BOARD_SIZE; col++) {
            Piece piece = getPiece(state, row, col);
            if (GET_PIECE_TYPE(piece) == KING && GET_PIECE_COLOR(piece) == color) {
                kingRow = row;
                kingCol = col;
                break;
            }
        }
        if (kingRow != -1) break;
    }
    
    if (kingRow == -1) return false; // King not found (shouldn't happen in a valid game)
    
    return isSquareAttacked(state, kingRow, kingCol, !color);
}

// Validate if a move is legal
bool isValidMove(const GameState *state, Move move) {
    int fromRow = move.fromRow;
    int fromCol = move.fromCol;
    int toRow = move.toRow;
    int toCol = move.toCol;
    
    // Check bounds
    if (fromRow < 0 || fromRow >= BOARD_SIZE || fromCol < 0 || fromCol >= BOARD_SIZE ||
        toRow < 0 || toRow >= BOARD_SIZE || toCol < 0 || toCol >= BOARD_SIZE) {
        return false;
    }
    
    Piece piece = getPiece(state, fromRow, fromCol);
    Piece targetPiece = getPiece(state, toRow, toCol);
    
    // Check if there's a piece to move and it's the current player's turn
    if (piece == EMPTY || GET_PIECE_COLOR(piece) != state->turn) {
        return false;
    }
    
    // Can't capture own pieces
    if (targetPiece != EMPTY && GET_PIECE_COLOR(targetPiece) == state->turn) {
        return false;
    }
    
    int pieceType = GET_PIECE_TYPE(piece);
    
    // Validate move based on piece type
    bool valid_piece_move = false;
    
    switch (pieceType) {
        case PAWN: {
            int direction = (state->turn == WHITE) ? 1 : -1;
            int startRow = (state->turn == WHITE) ? 1 : 6;
            
            // Forward move
            if (fromCol == toCol && targetPiece == EMPTY) {
                if (toRow == fromRow + direction) {
                    valid_piece_move = true;
                }
                // Double move from starting position
                else if (fromRow == startRow && toRow == fromRow + 2 * direction && 
                         getPiece(state, fromRow + direction, fromCol) == EMPTY) {
                    valid_piece_move = true;
                }
            }
            // Capture move (diagonal)
            else if (toRow == fromRow + direction && abs(toCol - fromCol) == 1) {
                // Normal capture
                if (targetPiece != EMPTY) {
                    valid_piece_move = true;
                }
                // En passant capture
                else if (state->enPassantCol == toCol) {
                    int epPawnRow = (state->turn == WHITE) ? 4 : 3;
                    if (fromRow == epPawnRow) {
                        valid_piece_move = true;
                    }
                }
            }
            
            // Check for valid promotion piece if it's a promotion move
            if (valid_piece_move && ((state->turn == WHITE && toRow == 7) || (state->turn == BLACK && toRow == 0))) {
                int promotionPiece = move.promotionPiece;
                if (promotionPiece < KNIGHT || promotionPiece > QUEEN) {
                    return false;
                }
            }
            break;
        }
        
        case KNIGHT: {
            int row_diff = abs(toRow - fromRow);
            int col_diff = abs(toCol - fromCol);
            
            valid_piece_move = (row_diff == 1 && col_diff == 2) || (row_diff == 2 && col_diff == 1);
            break;
        }
        
        case BISHOP: {
            int row_diff = abs(toRow - fromRow);
            int col_diff = abs(toCol - fromCol);
            
            if (row_diff == col_diff) {
                valid_piece_move = true;
                
                // Check for pieces in the path
                int row_step = (toRow > fromRow) ? 1 : -1;
                int col_step = (toCol > fromCol) ? 1 : -1;
                
                for (int i = 1; i < row_diff; i++) {
                    if (getPiece(state, fromRow + i * row_step, fromCol + i * col_step) != EMPTY) {
                        valid_piece_move = false;
                        break;
                    }
                }
            }
            break;
        }
        
        case ROOK: {
            if (fromRow == toRow || fromCol == toCol) {
                valid_piece_move = true;
                
                // Check for pieces in the path
                if (fromRow == toRow) {
                    int step = (toCol > fromCol) ? 1 : -1;
                    for (int col = fromCol + step; col != toCol; col += step) {
                        if (getPiece(state, fromRow, col) != EMPTY) {
                            valid_piece_move = false;
                            break;
                        }
                    }
                } else {
                    int step = (toRow > fromRow) ? 1 : -1;
                    for (int row = fromRow + step; row != toRow; row += step) {
                        if (getPiece(state, row, fromCol) != EMPTY) {
                            valid_piece_move = false;
                            break;
                        }
                    }
                }
            }
            break;
        }
        
        case QUEEN: {
            int row_diff = abs(toRow - fromRow);
            int col_diff = abs(toCol - fromCol);
            
            if ((fromRow == toRow || fromCol == toCol) || (row_diff == col_diff)) {
                valid_piece_move = true;
                
                // Check for pieces in the path
                if (fromRow == toRow) {
                    int step = (toCol > fromCol) ? 1 : -1;
                    for (int col = fromCol + step; col != toCol; col += step) {
                        if (getPiece(state, fromRow, col) != EMPTY) {
                            valid_piece_move = false;
                            break;
                        }
                    }
                } else if (fromCol == toCol) {
                    int step = (toRow > fromRow) ? 1 : -1;
                    for (int row = fromRow + step; row != toRow; row += step) {
                        if (getPiece(state, row, fromCol) != EMPTY) {
                            valid_piece_move = false;
                            break;
                        }
                    }
                } else { // Diagonal
                    int row_step = (toRow > fromRow) ? 1 : -1;
                    int col_step = (toCol > fromCol) ? 1 : -1;
                    
                    for (int i = 1; i < row_diff; i++) {
                        if (getPiece(state, fromRow + i * row_step, fromCol + i * col_step) != EMPTY) {
                            valid_piece_move = false;
                            break;
                        }
                    }
                }
            }
            break;
        }
        
        case KING: {
            int row_diff = abs(toRow - fromRow);
            int col_diff = abs(toCol - fromCol);
            
            // Normal king move
            if (row_diff <= 1 && col_diff <= 1) {
                valid_piece_move = true;
            }
            // Castling
            else if (row_diff == 0 && col_diff == 2 && !isInCheck(state, state->turn)) {
                int row = (state->turn == WHITE) ? 0 : 7;
                
                // Kingside castling
                if (toCol == 6 && state->castlingRights[state->turn][1]) {
                    Piece rook = getPiece(state, row, 7);
                    if (GET_PIECE_TYPE(rook) == ROOK && 
                        getPiece(state, row, 5) == EMPTY && 
                        getPiece(state, row, 6) == EMPTY &&
                        !isSquareAttacked(state, row, 5, !state->turn)) {
                        valid_piece_move = true;
                    }
                }
                // Queenside castling
                else if (toCol == 2 && state->castlingRights[state->turn][0]) {
                    Piece rook = getPiece(state, row, 0);
                    if (GET_PIECE_TYPE(rook) == ROOK && 
                        getPiece(state, row, 1) == EMPTY && 
                        getPiece(state, row, 2) == EMPTY && 
                        getPiece(state, row, 3) == EMPTY &&
                        !isSquareAttacked(state, row, 3, !state->turn)) {
                        valid_piece_move = true;
                    }
                }
            }
            break;
        }
    }
    
    if (!valid_piece_move) {
        return false;
    }
    
    // Make the move temporarily to check if it results in check
    GameState tempState = *state;
    Piece movingPiece = getPiece(&tempState, fromRow, fromCol);
    setPiece(&tempState, toRow, toCol, movingPiece);
    setPiece(&tempState, fromRow, fromCol, EMPTY);
    
    // Handle en passant capture
    if (pieceType == PAWN && toCol == state->enPassantCol && 
        ((state->turn == WHITE && fromRow == 4 && toRow == 5) ||
         (state->turn == BLACK && fromRow == 3 && toRow == 2))) {
        int capturedPawnRow = (state->turn == WHITE) ? 4 : 3;
        setPiece(&tempState, capturedPawnRow, toCol, EMPTY);
    }
    
    // Handle castling rook movement
    if (pieceType == KING && abs(toCol - fromCol) == 2) {
        int row = (state->turn == WHITE) ? 0 : 7;
        if (toCol == 6) { // Kingside
            setPiece(&tempState, row, 5, getPiece(&tempState, row, 7));
            setPiece(&tempState, row, 7, EMPTY);
        } else if (toCol == 2) { // Queenside
            setPiece(&tempState, row, 3, getPiece(&tempState, row, 0));
            setPiece(&tempState, row, 0, EMPTY);
        }
    }
    
    // Check if the move leaves the king in check
    bool inCheck = isInCheck(&tempState, state->turn);
    
    return !inCheck;
}

// Generate all legal moves for the current player
void generateMoves(const GameState *state, MoveList *moves) {
    moves->count = 0;
    
    for (int fromRow = 0; fromRow < BOARD_SIZE; fromRow++) {
        for (int fromCol = 0; fromCol < BOARD_SIZE; fromCol++) {
            Piece piece = getPiece(state, fromRow, fromCol);
            
            if (piece == EMPTY || GET_PIECE_COLOR(piece) != state->turn) {
                continue;
            }
            
            int pieceType = GET_PIECE_TYPE(piece);
            
            switch (pieceType) {
                case PAWN: {
                    int direction = (state->turn == WHITE) ? 1 : -1;
                    int startRow = (state->turn == WHITE) ? 1 : 6;
                    int promotionRow = (state->turn == WHITE) ? 7 : 0;
                    
                    // Forward moves
                    int newRow = fromRow + direction;
                    if (newRow >= 0 && newRow < BOARD_SIZE) {
                        // Single step forward
                        if (getPiece(state, newRow, fromCol) == EMPTY) {
                            if (newRow == promotionRow) {
                                // Pawn promotion
                                for (int p = KNIGHT; p <= QUEEN; p++) {
                                    Move move = {fromRow, fromCol, newRow, fromCol, p};
                                    if (isValidMove(state, move)) {
                                        moves->moves[moves->count++] = move;
                                    }
                                }
                            } else {
                                Move move = {fromRow, fromCol, newRow, fromCol, 0};
                                if (isValidMove(state, move)) {
                                    moves->moves[moves->count++] = move;
                                }
                            }
                            
                            // Double step from starting position
                            if (fromRow == startRow) {
                                newRow = fromRow + 2 * direction;
                                if (getPiece(state, newRow, fromCol) == EMPTY) {
                                    Move move = {fromRow, fromCol, newRow, fromCol, 0};
                                    if (isValidMove(state, move)) {
                                        moves->moves[moves->count++] = move;
                                    }
                                }
                            }
                        }
                        
                        // Captures
                        for (int dc = -1; dc <= 1; dc += 2) {
                            int newCol = fromCol + dc;
                            if (newCol >= 0 && newCol < BOARD_SIZE) {
                                Piece targetPiece = getPiece(state, newRow, newCol);
                                bool isCapture = (targetPiece != EMPTY && GET_PIECE_COLOR(targetPiece) != state->turn);
                                bool isEnPassant = (state->enPassantCol == newCol &&
                                                   ((state->turn == WHITE && fromRow == 4) ||
                                                    (state->turn == BLACK && fromRow == 3)));
                                
                                if (isCapture || isEnPassant) {
                                    if (newRow == promotionRow) {
                                        // Promotion with capture
                                        for (int p = KNIGHT; p <= QUEEN; p++) {
                                            Move move = {fromRow, fromCol, newRow, newCol, p};
                                            if (isValidMove(state, move)) {
                                                moves->moves[moves->count++] = move;
                                            }
                                        }
                                    } else {
                                        Move move = {fromRow, fromCol, newRow, newCol, 0};
                                        if (isValidMove(state, move)) {
                                            moves->moves[moves->count++] = move;
                                        }
                                    }
                                }
                            }
                        }
                    }
                    break;
                }
                
                case KNIGHT: {
                    const int knightMoves[8][2] = {
                        {-2, -1}, {-2, 1}, {-1, -2}, {-1, 2},
                        {1, -2}, {1, 2}, {2, -1}, {2, 1}
                    };
                    
                    for (int i = 0; i < 8; i++) {
                        int newRow = fromRow + knightMoves[i][0];
                        int newCol = fromCol + knightMoves[i][1];
                        
                        if (newRow >= 0 && newRow < BOARD_SIZE && newCol >= 0 && newCol < BOARD_SIZE) {
                            Move move = {fromRow, fromCol, newRow, newCol, 0};
                            if (isValidMove(state, move)) {
                                moves->moves[moves->count++] = move;
                            }
                        }
                    }
                    break;
                }
                
                case BISHOP: {
                    const int directions[4][2] = {
                        {-1, -1}, {-1, 1}, {1, -1}, {1, 1}
                    };
                    
                    for (int i = 0; i < 4; i++) {
                        int dr = directions[i][0];
                        int dc = directions[i][1];
                        
                        for (int dist = 1; dist < BOARD_SIZE; dist++) {
                            int newRow = fromRow + dist * dr;
                            int newCol = fromCol + dist * dc;
                            
                            if (newRow < 0 || newRow >= BOARD_SIZE || newCol < 0 || newCol >= BOARD_SIZE) {
                                break;
                            }
                            
                            Move move = {fromRow, fromCol, newRow, newCol, 0};
                            if (isValidMove(state, move)) {
                                moves->moves[moves->count++] = move;
                            }
                            
                            // Stop if we hit a piece
                            if (getPiece(state, newRow, newCol) != EMPTY) {
                                break;
                            }
                        }
                    }
                    break;
                }
                
                case ROOK: {
                    const int directions[4][2] = {
                        {-1, 0}, {1, 0}, {0, -1}, {0, 1}
                    };
                    
                    for (int i = 0; i < 4; i++) {
                        int dr = directions[i][0];
                        int dc = directions[i][1];
                        
                        for (int dist = 1; dist < BOARD_SIZE; dist++) {
                            int newRow = fromRow + dist * dr;
                            int newCol = fromCol + dist * dc;
                            
                            if (newRow < 0 || newRow >= BOARD_SIZE || newCol < 0 || newCol >= BOARD_SIZE) {
                                break;
                            }
                            
                            Move move = {fromRow, fromCol, newRow, newCol, 0};
                            if (isValidMove(state, move)) {
                                moves->moves[moves->count++] = move;
                            }
                            
                            // Stop if we hit a piece
                            if (getPiece(state, newRow, newCol) != EMPTY) {
                                break;
                            }
                        }
                    }
                    break;
                }
                
                case QUEEN: {
                    const int directions[8][2] = {
                        {-1, 0}, {1, 0}, {0, -1}, {0, 1},
                        {-1, -1}, {-1, 1}, {1, -1}, {1, 1}
                    };
                    
                    for (int i = 0; i < 8; i++) {
                        int dr = directions[i][0];
                        int dc = directions[i][1];
                        
                        for (int dist = 1; dist < BOARD_SIZE; dist++) {
                            int newRow = fromRow + dist * dr;
                            int newCol = fromCol + dist * dc;
                            
                            if (newRow < 0 || newRow >= BOARD_SIZE || newCol < 0 || newCol >= BOARD_SIZE) {
                                break;
                            }
                            
                            Move move = {fromRow, fromCol, newRow, newCol, 0};
                            if (isValidMove(state, move)) {
                                moves->moves[moves->count++] = move;
                            }
                            
                            // Stop if we hit a piece
                            if (getPiece(state, newRow, newCol) != EMPTY) {
                                break;
                            }
                        }
                    }
                    break;
                }
                
                case KING: {
                    // Normal king moves
                    const int kingMoves[8][2] = {
                        {-1, -1}, {-1, 0}, {-1, 1}, {0, -1},
                        {0, 1}, {1, -1}, {1, 0}, {1, 1}
                    };
                    
                    for (int i = 0; i < 8; i++) {
                        int newRow = fromRow + kingMoves[i][0];
                        int newCol = fromCol + kingMoves[i][1];
                        
                        if (newRow >= 0 && newRow < BOARD_SIZE && newCol >= 0 && newCol < BOARD_SIZE) {
                            Move move = {fromRow, fromCol, newRow, newCol, 0};
                            if (isValidMove(state, move)) {
                                moves->moves[moves->count++] = move;
                            }
                        }
                    }
                    
                    // Castling moves
                    if (!isInCheck(state, state->turn)) {
                        int row = (state->turn == WHITE) ? 0 : 7;
                        
                        // Kingside castling
                        if (state->castlingRights[state->turn][1]) {
                            Move move = {row, 4, row, 6, 0};
                            if (isValidMove(state, move)) {
                                moves->moves[moves->count++] = move;
                            }
                        }
                        
                        // Queenside castling
                        if (state->castlingRights[state->turn][0]) {
                            Move move = {row, 4, row, 2, 0};
                            if (isValidMove(state, move)) {
                                moves->moves[moves->count++] = move;
                            }
                        }
                    }
                    break;
                }
            }
        }
    }
}

// Check for checkmate (king in check with no legal moves)
bool isCheckmate(const GameState *state) {
    if (!isInCheck(state, state->turn)) {
        return false;
    }
    
    MoveList moves;
    generateMoves(state, &moves);
    
    return moves.count == 0;
}

// Check for stalemate (not in check but no legal moves)
bool isStalemate(const GameState *state) {
    if (isInCheck(state, state->turn)) {
        return false;
    }
    
    MoveList moves;
    generateMoves(state, &moves);
    
    return moves.count == 0;
}

// Check for various draw conditions
bool isDraw(const GameState *state) {
    return isStalemate(state) || isFiftyMoveDraw(state) || isInsufficientMaterial(state);
}

// Check for fifty-move rule draw
bool isFiftyMoveDraw(const GameState *state) {
    return state->halfMoveClock >= 100; // 50 full moves = 100 half-moves
}

// Check for insufficient material draw
bool isInsufficientMaterial(const GameState *state) {
    int pieceCounts[2][7] = {0}; // [color][piece type]
    
    // Count all pieces on the board
    for (int row = 0; row < BOARD_SIZE; row++) {
        for (int col = 0; col < BOARD_SIZE; col++) {
            Piece piece = getPiece(state, row, col);
            if (piece != EMPTY) {
                int pieceType = GET_PIECE_TYPE(piece);
                int color = GET_PIECE_COLOR(piece);
                pieceCounts[color][pieceType]++;
            }
        }
    }
    
    // King vs King
    if (pieceCounts[WHITE][PAWN] == 0 && pieceCounts[BLACK][PAWN] == 0 &&
        pieceCounts[WHITE][ROOK] == 0 && pieceCounts[BLACK][ROOK] == 0 &&
        pieceCounts[WHITE][QUEEN] == 0 && pieceCounts[BLACK][QUEEN] == 0) {
        
        // King vs King
        if (pieceCounts[WHITE][BISHOP] == 0 && pieceCounts[BLACK][BISHOP] == 0 &&
            pieceCounts[WHITE][KNIGHT] == 0 && pieceCounts[BLACK][KNIGHT] == 0) {
            return true;
        }
        
        // King + Bishop vs King
        if ((pieceCounts[WHITE][BISHOP] == 1 && pieceCounts[BLACK][BISHOP] == 0 && 
             pieceCounts[WHITE][KNIGHT] == 0 && pieceCounts[BLACK][KNIGHT] == 0) ||
            (pieceCounts[BLACK][BISHOP] == 1 && pieceCounts[WHITE][BISHOP] == 0 && 
             pieceCounts[BLACK][KNIGHT] == 0 && pieceCounts[WHITE][KNIGHT] == 0)) {
            return true;
        }
        
        // King + Knight vs King
        if ((pieceCounts[WHITE][KNIGHT] == 1 && pieceCounts[BLACK][KNIGHT] == 0 && 
             pieceCounts[WHITE][BISHOP] == 0 && pieceCounts[BLACK][BISHOP] == 0) ||
            (pieceCounts[BLACK][KNIGHT] == 1 && pieceCounts[WHITE][KNIGHT] == 0 && 
             pieceCounts[BLACK][BISHOP] == 0 && pieceCounts[WHITE][BISHOP] == 0)) {
            return true;
        }
        
        // King + Bishop vs King + Bishop (same color bishops)
        if (pieceCounts[WHITE][BISHOP] == 1 && pieceCounts[BLACK][BISHOP] == 1 &&
            pieceCounts[WHITE][KNIGHT] == 0 && pieceCounts[BLACK][KNIGHT] == 0) {
            // Check if bishops are on the same colored squares
            int whiteBishopColor = -1;
            int blackBishopColor = -1;
            
            for (int row = 0; row < BOARD_SIZE; row++) {
                for (int col = 0; col < BOARD_SIZE; col++) {
                    Piece piece = getPiece(state, row, col);
                    if (GET_PIECE_TYPE(piece) == BISHOP) {
                        int squareColor = (row + col) % 2;
                        if (GET_PIECE_COLOR(piece) == WHITE) {
                            whiteBishopColor = squareColor;
                        } else {
                            blackBishopColor = squareColor;
                        }
                    }
                }
            }
            
            if (whiteBishopColor == blackBishopColor) {
                return true;
            }
        }
    }
    
    return false;
}

// Check for threefold repetition
bool isThreefoldRepetition(const GameHistory *history) {
    if (history->historyCount < 8) { // Minimum moves needed for a repetition
        return false;
    }
    
    const GameState *currentState = &history->history[history->historyIndex].state;
    int repetitionCount = 1;
    
    // Check back through history for identical positions
    for (int i = history->historyIndex - 2; i >= 0; i -= 2) {
        const GameState *pastState = &history->history[i].state;
        
        bool identical = true;
        
        // Compare board positions
        for (int row = 0; row < BOARD_SIZE; row++) {
            for (int col = 0; col < BOARD_SIZE; col++) {
                if (pastState->board[row][col] != currentState->board[row][col]) {
                    identical = false;
                    break;
                }
            }
            if (!identical) break;
        }
        
        // Compare castling rights
        if (identical) {
            for (int color = 0; color < 2; color++) {
                for (int side = 0; side < 2; side++) {
                    if (pastState->castlingRights[color][side] != currentState->castlingRights[color][side]) {
                        identical = false;
                        break;
                    }
                }
                if (!identical) break;
            }
        }
        
        // Compare en passant possibilities
        if (identical && pastState->enPassantCol != currentState->enPassantCol) {
            identical = false;
        }
        
        if (identical) {
            repetitionCount++;
            if (repetitionCount >= 3) {
                return true;
            }
        }
    }
    
    return false;
}

// Execute a move and update the game state
bool makeMove(GameState *state, Move move, GameHistory *history) {
    if (!isValidMove(state, move)) {
        return false;
    }
    
    int fromRow = move.fromRow;
    int fromCol = move.fromCol;
    int toRow = move.toRow;
    int toCol = move.toCol;
    
    // Save current state for undo
    MoveHistory *hist = &history->history[history->historyCount];
    hist->state = *state;
    hist->lastMove = move;
    hist->capturedPiece = getPiece(state, toRow, toCol);
    hist->wasEnPassant = false;
    hist->wasCastling = false;
    hist->wasPromotion = false;
    hist->oldEnPassantCol = state->enPassantCol;
    hist->oldHalfMoveClock = state->halfMoveClock;
    memcpy(hist->oldCastlingRights, state->castlingRights, sizeof(state->castlingRights));
    
    Piece movingPiece = getPiece(state, fromRow, fromCol);
    int pieceType = GET_PIECE_TYPE(movingPiece);
    
    // Reset en passant
    state->enPassantCol = -1;
    
    // Update halfmove clock (reset on pawn move or capture)
    if (pieceType == PAWN || hist->capturedPiece != EMPTY) {
        state->halfMoveClock = 0;
    } else {
        state->halfMoveClock++;
    }
    
    // Handle special pawn moves
    if (pieceType == PAWN) {
        // Double move (set en passant)
        if (abs(toRow - fromRow) == 2) {
            state->enPassantCol = fromCol;
        }
        
        // En passant capture
        else if (fromCol != toCol && getPiece(state, toRow, toCol) == EMPTY) {
            int capturedPawnRow = (state->turn == WHITE) ? 4 : 3;
            hist->capturedPiece = getPiece(state, capturedPawnRow, toCol);
            setPiece(state, capturedPawnRow, toCol, EMPTY);
            hist->wasEnPassant = true;
        }
        
        // Promotion
        if ((state->turn == WHITE && toRow == 7) || (state->turn == BLACK && toRow == 0)) {
            movingPiece = CREATE_PIECE(move.promotionPiece, state->turn);
            hist->wasPromotion = true;
        }
    }
    
    // Handle castling
    if (pieceType == KING && abs(toCol - fromCol) == 2) {
        int row = (state->turn == WHITE) ? 0 : 7;
        hist->wasCastling = true;
        
        if (toCol == 6) { // Kingside
            setPiece(state, row, 5, getPiece(state, row, 7));
            setPiece(state, row, 7, EMPTY);
        } else if (toCol == 2) { // Queenside
            setPiece(state, row, 3, getPiece(state, row, 0));
            setPiece(state, row, 0, EMPTY);
        }
    }
    
    // Update castling rights
    if (pieceType == KING) {
        state->castlingRights[state->turn][0] = false;
        state->castlingRights[state->turn][1] = false;
    } else if (pieceType == ROOK) {
        if (fromRow == 0 && fromCol == 0) {
            state->castlingRights[WHITE][0] = false; // White queenside
        } else if (fromRow == 0 && fromCol == 7) {
            state->castlingRights[WHITE][1] = false; // White kingside
        } else if (fromRow == 7 && fromCol == 0) {
            state->castlingRights[BLACK][0] = false; // Black queenside
        } else if (fromRow == 7 && fromCol == 7) {
            state->castlingRights[BLACK][1] = false; // Black kingside
        }
    }
    
    // If a rook is captured, update castling rights
    if (hist->capturedPiece != EMPTY && GET_PIECE_TYPE(hist->capturedPiece) == ROOK) {
        if (toRow == 0 && toCol == 0) {
            state->castlingRights[WHITE][0] = false; // White queenside
        } else if (toRow == 0 && toCol == 7) {
            state->castlingRights[WHITE][1] = false; // White kingside
        } else if (toRow == 7 && toCol == 0) {
            state->castlingRights[BLACK][0] = false; // Black queenside
        } else if (toRow == 7 && toCol == 7) {
            state->castlingRights[BLACK][1] = false; // Black kingside
        }
    }
    
    // Make the move
    setPiece(state, fromRow, fromCol, EMPTY);
    setPiece(state, toRow, toCol, SET_PIECE_MOVED(movingPiece));
    
    // Update turn
    state->turn = !state->turn;
    
    // Update fullmove counter
    if (state->turn == WHITE) {
        state->fullMoveNumber++;
    }
    
    // Update history
    history->historyCount++;
    history->historyIndex = history->historyCount - 1;
    
    // Convert move to algebraic notation and add to PGN
    addMoveToPGN(state, move, history);
    
    return true;
}

// Undo the last move
void undoMove(GameState *state, GameHistory *history) {
    if (history->historyIndex < 0) {
        return; // Nothing to undo
    }
    
    MoveHistory *hist = &history->history[history->historyIndex];
    Move move = hist->lastMove;
    
    int fromRow = move.fromRow;
    int fromCol = move.fromCol;
    int toRow = move.toRow;
    int toCol = move.toCol;
    
    // Restore the moving piece (remove promotion if needed)
    Piece movingPiece = getPiece(state, toRow, toCol);
    if (hist->wasPromotion) {
        movingPiece = CREATE_PIECE(PAWN, state->turn == WHITE ? BLACK : WHITE);
    }
    
    // Restore the positions
    setPiece(state, fromRow, fromCol, movingPiece);
    setPiece(state, toRow, toCol, hist->capturedPiece);
    
    // Handle special moves
    if (hist->wasEnPassant) {
        int capturedPawnRow = (state->turn == BLACK) ? 4 : 3; // Opposite of current turn
        setPiece(state, capturedPawnRow, toCol, hist->capturedPiece);
        setPiece(state, toRow, toCol, EMPTY);
    } else if (hist->wasCastling) {
        int row = (state->turn == BLACK) ? 0 : 7; // Opposite of current turn
        
        if (toCol == 6) { // Kingside
            setPiece(state, row, 7, getPiece(state, row, 5));
            setPiece(state, row, 5, EMPTY);
        } else if (toCol == 2) { // Queenside
            setPiece(state, row, 0, getPiece(state, row, 3));
            setPiece(state, row, 3, EMPTY);
        }
    }
    
    // Restore state
    state->enPassantCol = hist->oldEnPassantCol;
    state->halfMoveClock = hist->oldHalfMoveClock;
    memcpy(state->castlingRights, hist->oldCastlingRights, sizeof(state->castlingRights));
    
    // Update turn
    state->turn = (state->turn == WHITE) ? BLACK : WHITE;
    
    // Update fullmove counter
    if (state->turn == BLACK) {
        state->fullMoveNumber--;
    }
    
    history->historyIndex--;
}

// Redo a previously undone move
void redoMove(GameState *state, GameHistory *history) {
    if (history->historyIndex >= history->historyCount - 1) {
        return; // Nothing to redo
    }
    
    history->historyIndex++;
    MoveHistory *hist = &history->history[history->historyIndex];
    *state = hist->state;
    
    // Execute the move again
    makeMove(state, hist->lastMove, history);
    
    // Fix the history index which was incremented again by makeMove
    history->historyCount--;
    history->historyIndex = history->historyCount - 1;
}

// Convert algebraic notation (e.g., "e2-e4") to a Move struct
void algebraicToMove(const char *algebraic, Move *move) {
    if (strlen(algebraic) < 5) {
        move->fromRow = -1; // Invalid
        return;
    }
    
    move->fromCol = algebraic[0] - 'a';
    move->fromRow = algebraic[1] - '1';
    move->toCol = algebraic[3] - 'a';
    move->toRow = algebraic[4] - '1';
    
    // Check for promotion
    if (strlen(algebraic) > 5 && algebraic[5] == '=') {
        char promotionChar = algebraic[6];
        switch (promotionChar) {
            case 'Q': move->promotionPiece = QUEEN; break;
            case 'R': move->promotionPiece = ROOK; break;
            case 'B': move->promotionPiece = BISHOP; break;
            case 'N': move->promotionPiece = KNIGHT; break;
            default: move->promotionPiece = 0;
        }
    } else {
        move->promotionPiece = 0;
    }
}

// Convert a Move struct to algebraic notation
void moveToAlgebraic(Move move, char *algebraic) {
    algebraic[0] = 'a' + move.fromCol;
    algebraic[1] = '1' + move.fromRow;
    algebraic[2] = '-';
    algebraic[3] = 'a' + move.toCol;
    algebraic[4] = '1' + move.toRow;
    
    // Add promotion if needed
    if (move.promotionPiece > 0) {
        algebraic[5] = '=';
        switch (move.promotionPiece) {
            case QUEEN: algebraic[6] = 'Q'; break;
            case ROOK: algebraic[6] = 'R'; break;
            case BISHOP: algebraic[6] = 'B'; break;
            case KNIGHT: algebraic[6] = 'N'; break;
        }
        algebraic[7] = '\0';
    } else {
        algebraic[5] = '\0';
    }
}

// Add a move to the PGN record
void addMoveToPGN(GameState *state, Move move, GameHistory *history) {
    char algebraic[10];
    moveToAlgebraic(move, algebraic);
    
    char pgnMove[20];
    if (state->turn == BLACK) { // The move was made by White
        sprintf(pgnMove, "%d. %s ", state->fullMoveNumber, algebraic);
    } else { // The move was made by Black
        sprintf(pgnMove, "%s ", algebraic);
    }
    
    // Append to PGN
    strcat(history->pgn + history->pgnLength, pgnMove);
    history->pgnLength += strlen(pgnMove);
}

// Save the game to a file in PGN format
bool saveGame(const GameState *state, const GameHistory *history, const char *filename) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        return false;
    }
    
    // Write PGN headers
    fprintf(file, "[Event \"Chess Game\"]\n");
    fprintf(file, "[Site \"Local Game\"]\n");
    fprintf(file, "[Date \"%s\"]\n", __DATE__);
    fprintf(file, "[Round \"?\"]\n");
    fprintf(file, "[White \"Player 1\"]\n");
    fprintf(file, "[Black \"Player 2\"]\n");
    
    // Write result based on game state
    if (isCheckmate(state)) {
        fprintf(file, "[Result \"%s\"]\n", state->turn == WHITE ? "0-1" : "1-0");
    } else if (isDraw(state)) {
        fprintf(file, "[Result \"1/2-1/2\"]\n");
    } else {
        fprintf(file, "[Result \"*\"]\n");
    }
    
    fprintf(file, "\n");
    fprintf(file, "%s", history->pgn);
    
    // Append result
    if (isCheckmate(state)) {
        fprintf(file, " %s", state->turn == WHITE ? "0-1" : "1-0");
    } else if (isDraw(state)) {
        fprintf(file, " 1/2-1/2");
    } else {
        fprintf(file, " *");
    }
    
    fclose(file);
    return true;
}

// Load a game from a PGN file
bool loadGame(GameState *state, GameHistory *history, const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        return false;
    }

    // Reset game state
    resetGame(state, history);

    // Read entire file
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *pgn = (char*)malloc(size + 1);
    if (!pgn) {
        fclose(file);
        return false;
    }
    fread(pgn, 1, size, file);
    pgn[size] = '\0';
    fclose(file);

    // Remove headers, comments, and variations
    char *clean = (char*)malloc(size + 1);
    if (!clean) {
        free(pgn);
        return false;
    }

    bool in_brace = false;
    bool in_semi = false;
    int paren = 0;
    size_t j = 0;
    for (size_t i = 0; pgn[i]; i++) {
        char c = pgn[i];
        if (in_brace) { if (c == '}') in_brace = false; continue; }
        if (in_semi) { if (c == '\n' || c == '\r') in_semi = false; continue; }
        if (c == '{') { in_brace = true; continue; }
        if (c == ';') { in_semi = true; continue; }
        if (c == '(') { paren++; continue; }
        if (c == ')') { if (paren > 0) paren--; continue; }
        if (paren > 0) continue;
        if (c == '[') { while (pgn[i] && pgn[i] != ']') i++; continue; }
        clean[j++] = c;
    }
    clean[j] = '\0';

    // Tokenize
    char *token = strtok(clean, " \t\r\n");
    while (token) {
        if (strchr(token, '.')) { token = strtok(NULL, " \t\r\n"); continue; }
        if (!strcmp(token, "1-0") || !strcmp(token, "0-1") ||
            !strcmp(token, "1/2-1/2") || !strcmp(token, "*")) {
            token = strtok(NULL, " \t\r\n");
            continue;
        }

        Move move;
        algebraicToMove(token, &move);
        if (!makeMove(state, move, history)) {
            free(pgn);
            free(clean);
            return false;
        }

        token = strtok(NULL, " \t\r\n");
    }

    free(pgn);
    free(clean);
    return true;
}

// Utility function to get a character representing a piece
char getPieceChar(Piece piece) {
    if (piece == EMPTY) {
        return ' ';
    }
    
    char pieceChars[] = " PNBRQK";
    char c = pieceChars[GET_PIECE_TYPE(piece)];
    
    return (GET_PIECE_COLOR(piece) == WHITE) ? c : tolower(c);
}

// Print the board to the console (for debugging)
void printBoard(const GameState *state) {
    printf("  a b c d e f g h\n");
    printf(" +-----------------+\n");
    
    for (int row = BOARD_SIZE - 1; row >= 0; row--) {
        printf("%d|", row + 1);
        
        for (int col = 0; col < BOARD_SIZE; col++) {
            printf(" %c", getPieceChar(getPiece(state, row, col)));
        }
        
        printf(" |\n");
    }
    
    printf(" +-----------------+\n");
    
    printf("Turn: %s\n", state->turn == WHITE ? "White" : "Black");
    
    if (isInCheck(state, state->turn)) {
        printf("CHECK!\n");
    }
}