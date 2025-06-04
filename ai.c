#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include "ai.h"

// Piece-Square Tables (adapted from chess programming wiki)
// Values are for white pieces. For black pieces, the tables are flipped.

// Pawn position table
const int PAWN_TABLE[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
    50, 50, 50, 50, 50, 50, 50, 50,
    10, 10, 20, 30, 30, 20, 10, 10,
     5,  5, 10, 25, 25, 10,  5,  5,
     0,  0,  0, 20, 20,  0,  0,  0,
     5, -5,-10,  0,  0,-10, -5,  5,
     5, 10, 10,-20,-20, 10, 10,  5,
     0,  0,  0,  0,  0,  0,  0,  0
};

// Knight position table
const int KNIGHT_TABLE[64] = {
    -50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50
};

// Bishop position table
const int BISHOP_TABLE[64] = {
    -20,-10,-10,-10,-10,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0, 10, 10, 10, 10,  0,-10,
    -10,  5,  5, 10, 10,  5,  5,-10,
    -10,  0,  5, 10, 10,  5,  0,-10,
    -10,  5,  5,  5,  5,  5,  5,-10,
    -10,  0,  5,  0,  0,  5,  0,-10,
    -20,-10,-10,-10,-10,-10,-10,-20
};

// Rook position table
const int ROOK_TABLE[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
     5, 10, 10, 10, 10, 10, 10,  5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
     0,  0,  0,  5,  5,  0,  0,  0
};

// Queen position table
const int QUEEN_TABLE[64] = {
    -20,-10,-10, -5, -5,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5,  5,  5,  5,  0,-10,
     -5,  0,  5,  5,  5,  5,  0, -5,
      0,  0,  5,  5,  5,  5,  0, -5,
    -10,  5,  5,  5,  5,  5,  0,-10,
    -10,  0,  5,  0,  0,  0,  0,-10,
    -20,-10,-10, -5, -5,-10,-10,-20
};

// King position table (middle game)
const int KING_TABLE_MIDDLE[64] = {
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -20,-30,-30,-40,-40,-30,-30,-20,
    -10,-20,-20,-20,-20,-20,-20,-10,
     20, 20,  0,  0,  0,  0, 20, 20,
     20, 30, 10,  0,  0, 10, 30, 20
};

// King position table (end game)
const int KING_TABLE_END[64] = {
    -50,-40,-30,-20,-20,-30,-40,-50,
    -30,-20,-10,  0,  0,-10,-20,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-30,  0,  0,  0,  0,-30,-30,
    -50,-30,-30,-30,-30,-30,-30,-50
};

// Piece values (centipawns)
#define PAWN_VALUE 100
#define KNIGHT_VALUE 320
#define BISHOP_VALUE 330
#define ROOK_VALUE 500
#define QUEEN_VALUE 900
#define KING_VALUE 20000

// AI selection of the best move using minimax with alpha-beta pruning
Move getBestMove(GameState *state, AIDifficulty difficulty) {
    MoveList moves;
    generateMoves(state, &moves);
    
    if (moves.count == 0) {
        // No legal moves
        Move nullMove = {-1, -1, -1, -1, 0};
        return nullMove;
    }
    
    // Seed random number generator
    srand((unsigned int)time(NULL));
    
    // Initialize history for undoing moves
    GameHistory history;
    memset(&history, 0, sizeof(GameHistory));
    
    int bestScore = INT_MIN;
    int bestMoveIndex = 0;
    int alpha = INT_MIN;
    int beta = INT_MAX;
    
    // Calculate scores for all moves
    int *scores = malloc(moves.count * sizeof(int));
    
    for (int i = 0; i < moves.count; i++) {
        // Make move
        makeMove(state, moves.moves[i], &history);
        
        // Evaluate with minimax
        scores[i] = -minimax(state, difficulty - 1, -beta, -alpha, false, &history);
        
        // Undo move
        undoMove(state, &history);
        
        if (scores[i] > bestScore) {
            bestScore = scores[i];
            bestMoveIndex = i;
        }
        
        // Update alpha for alpha-beta pruning
        if (scores[i] > alpha) {
            alpha = scores[i];
        }
    }
    
    // Find all moves that have a score close to the best score
    int threshold = 10; // Within 0.1 pawns of the best move
    int goodMoveCount = 0;
    int *goodMoves = malloc(moves.count * sizeof(int));
    
    for (int i = 0; i < moves.count; i++) {
        if (scores[i] >= bestScore - threshold) {
            goodMoves[goodMoveCount++] = i;
        }
    }
    
    // Randomly select from good moves to add variety
    if (goodMoveCount > 1) {
        bestMoveIndex = goodMoves[rand() % goodMoveCount];
    }
    
    free(scores);
    free(goodMoves);
    
    return moves.moves[bestMoveIndex];
}

// Minimax algorithm with alpha-beta pruning
int minimax(GameState *state, int depth, int alpha, int beta, bool maximizing, GameHistory *history) {
    // Check for checkmate, stalemate, or draw
    if (isCheckmate(state)) {
        return maximizing ? -10000 : 10000;
    }
    
    if (isStalemate(state) || isDraw(state)) {
        return 0;
    }
    
    // Base case: reached max depth
    if (depth <= 0) {
        return quiescenceSearch(state, alpha, beta, history);
    }
    
    MoveList moves;
    generateMoves(state, &moves);
    
    if (maximizing) {
        int maxEval = INT_MIN;
        
        for (int i = 0; i < moves.count; i++) {
            makeMove(state, moves.moves[i], history);
            int eval = minimax(state, depth - 1, alpha, beta, false, history);
            undoMove(state, history);
            
            maxEval = (eval > maxEval) ? eval : maxEval;
            alpha = (alpha > maxEval) ? alpha : maxEval;
            
            if (beta <= alpha) {
                break; // Beta cutoff
            }
        }
        
        return maxEval;
    } else {
        int minEval = INT_MAX;
        
        for (int i = 0; i < moves.count; i++) {
            makeMove(state, moves.moves[i], history);
            int eval = minimax(state, depth - 1, alpha, beta, true, history);
            undoMove(state, history);
            
            minEval = (eval < minEval) ? eval : minEval;
            beta = (beta < minEval) ? beta : minEval;
            
            if (beta <= alpha) {
                break; // Alpha cutoff
            }
        }
        
        return minEval;
    }
}

// Quiescence search to avoid horizon effect
int quiescenceSearch(GameState *state, int alpha, int beta, GameHistory *history) {
    int standPat = evaluatePosition(state);
    
    if (standPat >= beta) {
        return beta;
    }
    
    if (alpha < standPat) {
        alpha = standPat;
    }
    
    MoveList moves;
    generateMoves(state, &moves);
    
    // Filter for captures only
    MoveList captureMoves;
    captureMoves.count = 0;
    
    for (int i = 0; i < moves.count; i++) {
        Move move = moves.moves[i];
        Piece targetPiece = getPiece(state, move.toRow, move.toCol);
        
        // Include only captures
        if (targetPiece != EMPTY) {
            captureMoves.moves[captureMoves.count++] = move;
        }
    }
    
    for (int i = 0; i < captureMoves.count; i++) {
        makeMove(state, captureMoves.moves[i], history);
        int score = -quiescenceSearch(state, -beta, -alpha, history);
        undoMove(state, history);
        
        if (score >= beta) {
            return beta;
        }
        
        if (score > alpha) {
            alpha = score;
        }
    }
    
    return alpha;
}

// Evaluate the current position
int evaluatePosition(const GameState *state) {
    // Calculate material and positional scores
    int score = materialScore(state);
    
    // Add mobility factor
    score += mobilityScore(state);
    
    // Add pawn structure evaluation
    score += pawnStructureScore(state);
    
    // Add king safety evaluation
    score += kingSafetyScore(state);
    
    // Add center control evaluation
    score += centerControlScore(state);
    
    // Perspective adjustment: positive is good for the current player
    return (state->turn == WHITE) ? score : -score;
}

// Calculate total material score and piece positioning
int materialScore(const GameState *state) {
    int score = 0;
    bool endgame = isEndgame(state);
    
    for (int row = 0; row < BOARD_SIZE; row++) {
        for (int col = 0; col < BOARD_SIZE; col++) {
            Piece piece = getPiece(state, row, col);
            if (piece == EMPTY) continue;
            
            int pieceType = GET_PIECE_TYPE(piece);
            int color = GET_PIECE_COLOR(piece);
            int square = (color == WHITE) ? (row * 8 + col) : ((7 - row) * 8 + col);
            
            // Material value
            int value = 0;
            switch (pieceType) {
                case PAWN: value = PAWN_VALUE; break;
                case KNIGHT: value = KNIGHT_VALUE; break;
                case BISHOP: value = BISHOP_VALUE; break;
                case ROOK: value = ROOK_VALUE; break;
                case QUEEN: value = QUEEN_VALUE; break;
                case KING: value = KING_VALUE; break;
            }
            
            // Position value
            int posValue = 0;
            switch (pieceType) {
                case PAWN: posValue = PAWN_TABLE[square]; break;
                case KNIGHT: posValue = KNIGHT_TABLE[square]; break;
                case BISHOP: posValue = BISHOP_TABLE[square]; break;
                case ROOK: posValue = ROOK_TABLE[square]; break;
                case QUEEN: posValue = QUEEN_TABLE[square]; break;
                case KING: 
                    posValue = endgame ? KING_TABLE_END[square] : KING_TABLE_MIDDLE[square]; 
                    break;
            }
            
            // Add or subtract based on piece color
            if (color == WHITE) {
                score += value + posValue;
            } else {
                score -= value + posValue;
            }
        }
    }
    
    return score;
}

// Evaluate piece mobility
int mobilityScore(const GameState *state) {
    const int MOBILITY_WEIGHT = 10;
    
    // Count legal moves for each side
    Color originalTurn = state->turn;
    int whiteMoves = 0;
    int blackMoves = 0;
    
    // Create a temporary state
    GameState tempState = *state;
    
    // Count white moves
    tempState.turn = WHITE;
    MoveList whiteMoveList;
    generateMoves(&tempState, &whiteMoveList);
    whiteMoves = whiteMoveList.count;
    
    // Count black moves
    tempState.turn = BLACK;
    MoveList blackMoveList;
    generateMoves(&tempState, &blackMoveList);
    blackMoves = blackMoveList.count;
    
    // Calculate mobility score
    int mobilityScore = (whiteMoves - blackMoves) * MOBILITY_WEIGHT;
    
    return mobilityScore;
}

// Evaluate pawn structure
int pawnStructureScore(const GameState *state) {
    int score = 0;
    const int DOUBLED_PAWN_PENALTY = -10;
    const int ISOLATED_PAWN_PENALTY = -20;
    const int PASSED_PAWN_BONUS = 30;
    
    // Check each file for white and black pawns
    for (int col = 0; col < BOARD_SIZE; col++) {
        int whitePawnsInFile = 0;
        int blackPawnsInFile = 0;
        int whiteLowestPawn = -1;
        int blackHighestPawn = -1;
        
        for (int row = 0; row < BOARD_SIZE; row++) {
            Piece piece = getPiece(state, row, col);
            
            if (GET_PIECE_TYPE(piece) == PAWN) {
                if (GET_PIECE_COLOR(piece) == WHITE) {
                    whitePawnsInFile++;
                    if (whiteLowestPawn == -1 || row < whiteLowestPawn) {
                        whiteLowestPawn = row;
                    }
                } else {
                    blackPawnsInFile++;
                    if (blackHighestPawn == -1 || row > blackHighestPawn) {
                        blackHighestPawn = row;
                    }
                }
            }
        }
        
        // Doubled pawns penalty
        if (whitePawnsInFile > 1) {
            score += DOUBLED_PAWN_PENALTY * (whitePawnsInFile - 1);
        }
        if (blackPawnsInFile > 1) {
            score -= DOUBLED_PAWN_PENALTY * (blackPawnsInFile - 1);
        }
        
        // Isolated pawns penalty
        if (whitePawnsInFile > 0) {
            bool isIsolated = true;
            
            if (col > 0) {
                for (int row = 0; row < BOARD_SIZE; row++) {
                    Piece leftPiece = getPiece(state, row, col - 1);
                    if (GET_PIECE_TYPE(leftPiece) == PAWN && GET_PIECE_COLOR(leftPiece) == WHITE) {
                        isIsolated = false;
                        break;
                    }
                }
            }
            
            if (isIsolated && col < BOARD_SIZE - 1) {
                for (int row = 0; row < BOARD_SIZE; row++) {
                    Piece rightPiece = getPiece(state, row, col + 1);
                    if (GET_PIECE_TYPE(rightPiece) == PAWN && GET_PIECE_COLOR(rightPiece) == WHITE) {
                        isIsolated = false;
                        break;
                    }
                }
            }
            
            if (isIsolated) {
                score += ISOLATED_PAWN_PENALTY;
            }
        }
        
        if (blackPawnsInFile > 0) {
            bool isIsolated = true;
            
            if (col > 0) {
                for (int row = 0; row < BOARD_SIZE; row++) {
                    Piece leftPiece = getPiece(state, row, col - 1);
                    if (GET_PIECE_TYPE(leftPiece) == PAWN && GET_PIECE_COLOR(leftPiece) == BLACK) {
                        isIsolated = false;
                        break;
                    }
                }
            }
            
            if (isIsolated && col < BOARD_SIZE - 1) {
                for (int row = 0; row < BOARD_SIZE; row++) {
                    Piece rightPiece = getPiece(state, row, col + 1);
                    if (GET_PIECE_TYPE(rightPiece) == PAWN && GET_PIECE_COLOR(rightPiece) == BLACK) {
                        isIsolated = false;
                        break;
                    }
                }
            }
            
            if (isIsolated) {
                score -= ISOLATED_PAWN_PENALTY;
            }
        }
        
        // Passed pawns bonus
        if (whitePawnsInFile > 0 && whiteLowestPawn != -1) {
            bool isPassed = true;
            
            // Check if there are any black pawns that can block this pawn
            for (int checkCol = col - 1; checkCol <= col + 1; checkCol++) {
                if (checkCol < 0 || checkCol >= BOARD_SIZE) continue;
                
                for (int row = whiteLowestPawn + 1; row < BOARD_SIZE; row++) {
                    Piece piece = getPiece(state, row, checkCol);
                    if (GET_PIECE_TYPE(piece) == PAWN && GET_PIECE_COLOR(piece) == BLACK) {
                        isPassed = false;
                        break;
                    }
                }
                
                if (!isPassed) break;
            }
            
            if (isPassed) {
                score += PASSED_PAWN_BONUS + (whiteLowestPawn * 5);
            }
        }
        
        if (blackPawnsInFile > 0 && blackHighestPawn != -1) {
            bool isPassed = true;
            
            // Check if there are any white pawns that can block this pawn
            for (int checkCol = col - 1; checkCol <= col + 1; checkCol++) {
                if (checkCol < 0 || checkCol >= BOARD_SIZE) continue;
                
                for (int row = blackHighestPawn - 1; row >= 0; row--) {
                    Piece piece = getPiece(state, row, checkCol);
                    if (GET_PIECE_TYPE(piece) == PAWN && GET_PIECE_COLOR(piece) == WHITE) {
                        isPassed = false;
                        break;
                    }
                }
                
                if (!isPassed) break;
            }
            
            if (isPassed) {
                score -= PASSED_PAWN_BONUS + ((7 - blackHighestPawn) * 5);
            }
        }
    }
    
    return score;
}

// Evaluate king safety
int kingSafetyScore(const GameState *state) {
    const int KING_SHIELD_BONUS = 10;
    const int KING_EXPOSED_PENALTY = -15;
    
    int score = 0;
    int whiteKingRow = -1, whiteKingCol = -1;
    int blackKingRow = -1, blackKingCol = -1;
    
    // Find kings
    for (int row = 0; row < BOARD_SIZE; row++) {
        for (int col = 0; col < BOARD_SIZE; col++) {
            Piece piece = getPiece(state, row, col);
            if (GET_PIECE_TYPE(piece) == KING) {
                if (GET_PIECE_COLOR(piece) == WHITE) {
                    whiteKingRow = row;
                    whiteKingCol = col;
                } else {
                    blackKingRow = row;
                    blackKingCol = col;
                }
            }
        }
    }
    
    // Early return if kings not found (shouldn't happen)
    if (whiteKingRow == -1 || blackKingRow == -1) {
        return 0;
    }
    
    // Evaluate white king safety
    if (!isEndgame(state)) {
        // Castled kingside
        if (whiteKingCol >= 6) {
            for (int col = whiteKingCol - 1; col <= whiteKingCol + 1; col++) {
                if (col < 0 || col >= BOARD_SIZE) continue;
                
                Piece piece = getPiece(state, whiteKingRow + 1, col);
                if (GET_PIECE_TYPE(piece) == PAWN && GET_PIECE_COLOR(piece) == WHITE) {
                    score += KING_SHIELD_BONUS;
                } else {
                    score += KING_EXPOSED_PENALTY;
                }
            }
        }
        // Castled queenside
        else if (whiteKingCol <= 2) {
            for (int col = whiteKingCol - 1; col <= whiteKingCol + 1; col++) {
                if (col < 0 || col >= BOARD_SIZE) continue;
                
                Piece piece = getPiece(state, whiteKingRow + 1, col);
                if (GET_PIECE_TYPE(piece) == PAWN && GET_PIECE_COLOR(piece) == WHITE) {
                    score += KING_SHIELD_BONUS;
                } else {
                    score += KING_EXPOSED_PENALTY;
                }
            }
        }
        // King in center (bad)
        else if (whiteKingCol >= 3 && whiteKingCol <= 5) {
            score += KING_EXPOSED_PENALTY * 2;
        }
    }
    
    // Evaluate black king safety
    if (!isEndgame(state)) {
        // Castled kingside
        if (blackKingCol >= 6) {
            for (int col = blackKingCol - 1; col <= blackKingCol + 1; col++) {
                if (col < 0 || col >= BOARD_SIZE) continue;
                
                Piece piece = getPiece(state, blackKingRow - 1, col);
                if (GET_PIECE_TYPE(piece) == PAWN && GET_PIECE_COLOR(piece) == BLACK) {
                    score -= KING_SHIELD_BONUS;
                } else {
                    score -= KING_EXPOSED_PENALTY;
                }
            }
        }
        // Castled queenside
        else if (blackKingCol <= 2) {
            for (int col = blackKingCol - 1; col <= blackKingCol + 1; col++) {
                if (col < 0 || col >= BOARD_SIZE) continue;
                
                Piece piece = getPiece(state, blackKingRow - 1, col);
                if (GET_PIECE_TYPE(piece) == PAWN && GET_PIECE_COLOR(piece) == BLACK) {
                    score -= KING_SHIELD_BONUS;
                } else {
                    score -= KING_EXPOSED_PENALTY;
                }
            }
        }
        // King in center (bad)
        else if (blackKingCol >= 3 && blackKingCol <= 5) {
            score -= KING_EXPOSED_PENALTY * 2;
        }
    }
    
    return score;
}

// Evaluate center control
int centerControlScore(const GameState *state) {
    const int CENTER_CONTROL_BONUS = 10;
    
    int score = 0;
    
    // Center squares
    const int centerSquares[4][2] = {
        {3, 3}, {3, 4}, {4, 3}, {4, 4}
    };
    
    // Count attacks on center squares
    for (int i = 0; i < 4; i++) {
        int row = centerSquares[i][0];
        int col = centerSquares[i][1];
        
        if (isSquareAttacked(state, row, col, WHITE)) {
            score += CENTER_CONTROL_BONUS;
        }
        
        if (isSquareAttacked(state, row, col, BLACK)) {
            score -= CENTER_CONTROL_BONUS;
        }
        
        // Bonus for pieces in the center
        Piece piece = getPiece(state, row, col);
        if (piece != EMPTY) {
            if (GET_PIECE_COLOR(piece) == WHITE) {
                score += CENTER_CONTROL_BONUS;
            } else {
                score -= CENTER_CONTROL_BONUS;
            }
        }
    }
    
    return score;
}

// Determine if the game is in the endgame phase
bool isEndgame(const GameState *state) {
    int whiteValue = 0;
    int blackValue = 0;
    
    for (int row = 0; row < BOARD_SIZE; row++) {
        for (int col = 0; col < BOARD_SIZE; col++) {
            Piece piece = getPiece(state, row, col);
            if (piece == EMPTY) continue;
            
            int pieceType = GET_PIECE_TYPE(piece);
            int pieceValue = 0;
            
            switch (pieceType) {
                case PAWN: pieceValue = PAWN_VALUE; break;
                case KNIGHT: pieceValue = KNIGHT_VALUE; break;
                case BISHOP: pieceValue = BISHOP_VALUE; break;
                case ROOK: pieceValue = ROOK_VALUE; break;
                case QUEEN: pieceValue = QUEEN_VALUE; break;
                case KING: continue; // Skip king
            }
            
            if (GET_PIECE_COLOR(piece) == WHITE) {
                whiteValue += pieceValue;
            } else {
                blackValue += pieceValue;
            }
        }
    }
    
    // Endgame if both players have less than a queen + rook in material
    const int ENDGAME_THRESHOLD = QUEEN_VALUE + ROOK_VALUE;
    return (whiteValue < ENDGAME_THRESHOLD && blackValue < ENDGAME_THRESHOLD);
}