#include "defs.h"
#include "chess.hpp"

using namespace Chess;

int quiesce(Board& board, int alpha, int beta)
{
    int stand_pat = eval(board);
    if (stand_pat >= beta){
        return beta;
    }
    if (stand_pat > alpha){
        alpha = stand_pat;
    }
    Movelist moveslist;
    Movegen::legalmoves<CAPTURE>(board, moveslist);
    for (int i = 0; i<int(moveslist.size);i++){
        
        Move move = moveslist[i].move;
        board.makeMove(move);
        int score = -quiesce(board,-beta, -alpha);
        board.unmakeMove(move);
        
        if (score >= beta){
            return score;
        }
        if (score > alpha){
            alpha = score;
        }
    }
    
    return alpha;
}


int alpha_beta (Board& board, PV& pv, int alpha, int beta, int depth){
    
    if (depth == 0){
        return quiesce(board, alpha, beta);
    }

    PV local_pv;
    Movelist moveslist;
    Movegen::legalmoves<ALL>(board, moveslist);
    
    if (moveslist.size == 0){
        Color oppositeSide = ~board.sideToMove;
        if (board.isSquareAttacked(oppositeSide, board.KingSQ(board.sideToMove))){
            return -MATE_SCORE;
        };
        return DRAW_SCORE;
    }

    for (int i = 0; i<int(moveslist.size);i++){
        
        Move move = moveslist[i].move;
        board.makeMove(move);
        int score = -alpha_beta(board, local_pv, -beta, -alpha, depth-1);
        if (score >= beta){
            return beta;
        }
        if (score > alpha){
            alpha = score;
            pv.load_from(move, pv);
        }
        board.unmakeMove(move);
    }
    
    return alpha;
}