#pragma once

#include <algorithm>
#include <chrono>
#include "eval.h"
#include "tt.h"
#include "move_ordering.h"
#include "pv.h"

using namespace Chess;

std::chrono::microseconds get_current_time()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch());
}

TranspositionTable transposition_table(16 * 1024 * 1024);
PVHistory pv_history;

class Search
{
private:
    std::chrono::microseconds wtime;
    std::chrono::microseconds btime;
    std::chrono::microseconds winc;
    std::chrono::microseconds binc;
    std::chrono::microseconds lastTime = get_current_time();
    std::chrono::microseconds timePerMove;
    bool stop = false;

public:
    /*
    
        Time Management
    
    */
    void stop_timer(){
        this->stop = true;
    }
    void set_timer(std::chrono::microseconds wtime, std::chrono::microseconds btime, std::chrono::microseconds timePerMove, std::chrono::microseconds winc = std::chrono::microseconds(0), std::chrono::microseconds binc = std::chrono::microseconds(0))
    {
        /*std::cout << wtime.count() << std::endl;
        std::cout << btime.count() << std::endl;
        std::cout << timePerMove.count() << std::endl;
        std::cout << winc.count() << std::endl;
        std::cout << binc.count() << std::endl;*/

        this->wtime = wtime;
        this->btime = btime;
        this->winc = winc;
        this->binc = binc;
        this->timePerMove = timePerMove;
        this->lastTime = get_current_time();
        this->stop = false;
    }
    bool check_time()
    {
        if (this->stop == true)
        {
            return true;
        }
        if (get_current_time() - this->lastTime >= this->timePerMove)
        {
            // std::cout << get_current_time().count() - this->lastTime.count() << std::endl;
            this->stop = true;
            this->lastTime = get_current_time();
            return true;
        }
        return false;
    }


    /*
        Qsearch
    */
    int quiesce(Board& board, int alpha, int beta, int ply, bool is_timed = true)
    {
        if (this->check_time() && is_timed){
            return 0;
        }

        if (ply >= MAX_DEPTH)
            return eval(board);

        int best = eval(board);
        if (best >= beta)
        {
            return beta;
        }
        if (best > alpha)
        {
            alpha = best;
        }
        Movelist moveslist;

        Movegen::legalmoves<CAPTURE>(board, moveslist);
        for (int i = 0; i < int(moveslist.size); i++)
        {

            Move move = moveslist[i].move;
            board.makeMove(move);
            int score = -this->quiesce(board, -beta, -alpha, ply + 1, is_timed);
            board.unmakeMove(move);

            if (score > best)
            {
                best = score;
                if (score > alpha)
                {
                    alpha = score;
                    if (score >= beta)
                    {
                        break;
                    }
                }
            }
        }

        return alpha;
    }

    /*
        Negamax with alpha beta.
    */
    int alpha_beta(Board& board, PV &pv, int alpha, int beta, int depth, int ply, bool DO_NULL, bool is_timed = true)
    {
        if (board.isRepetition() || (this->check_time() && is_timed)){
            return 0;
        }
        int best = -BEST_SCORE;
        bool isPVNode = beta-alpha != 1;

        if (ply > MAX_DEPTH - 1)
        {
            return eval(board);
        }
        if (depth == 0)
        {
            return this->quiesce(board, alpha, beta, ply, is_timed);
        }

        PV local_pv;
        
        /* 
        Null Move pruning
        */
        if (depth >= 3 && DO_NULL && !board.isSquareAttacked(board.sideToMove == Black ? White : Black, board.KingSQ(board.sideToMove))){
            board.makeNullMove();
            best = -this->alpha_beta(board, local_pv, -beta, -beta+1, depth-2, ply + 1, false, is_timed);
            board.unmakeNullMove();
            if (this->check_time() && is_timed){
                return 0;
            }
            if (best >= beta){
                return beta;
            }
        }

        best = -BEST_SCORE;
        
        int oldAlpha = alpha;
        
        Movelist moveslist;
        Movegen::legalmoves<ALL>(board, moveslist);

        // Checkmate and stalemate detection
        if (moveslist.size == 0)
        {
            if (board.isSquareAttacked(~board.sideToMove, board.KingSQ(board.sideToMove)))
            {
                return -MATE_SCORE + ply;
            }
            return DRAW_SCORE;
        }

        // TT entry
        U64 hashKey = board.hashKey;
        TTEntry tte = transposition_table.probeEntry(hashKey);

        // Move scoring
        MOVE_ORDER::give_moves_score(moveslist, tte.move, board);
        // Sorting moves
        std::sort(moveslist.list, moveslist.list + moveslist.size, [](ExtMove a, ExtMove b)
                  { return (a.value > b.value); });

        // TT flag and scoring
        if ((ply != 0) && (hashKey == tte.key) && (tte.depth >= depth))
        {
            if (tte.flag == EXACTBOUND)
            {
                return tte.score;
            }
            else if (tte.flag == LOWERBOUND)
            {
                alpha = std::max(alpha, tte.score);
            }
            else if (tte.flag == UPPERBOUND)
            {
                beta = std::min(beta, tte.score);
            }
            if (alpha >= beta)
            {
                return tte.score;
            }
        }

        for (int i = 0; i < int(moveslist.size); i++)
        {
            Move move = moveslist[i].move;
            // std::cout << convertMoveToUci(move) << std::endl;
            board.makeMove(move);
            int score;

            /*
                Null window search if non pv node.
            */
            if (!isPVNode || i > 0){
                score = -this->alpha_beta(board, local_pv, -alpha-1, -alpha, depth-1, ply+1, DO_NULL, is_timed);
            }

            /*
                Principal variation search (PVS).
            */
            if (isPVNode && ((score > alpha && score < beta) || i == 0)){
                score = -this->alpha_beta(board, local_pv, -beta, -alpha, depth - 1, ply + 1, DO_NULL, is_timed);
            }
            
            board.unmakeMove(move);

            if (score > best)
            {
                best = score;
                if (score > alpha)
                {
                    alpha = score;
                    pv.load_from(move, pv);
                    if (score >= beta)
                    {
                        break;
                    }
                }
            }
        }

        Flag bound = NONEBOUND;

        if (best <= oldAlpha)
        {
            bound = UPPERBOUND;
        }
        else if (best >= beta)
        {
            bound = LOWERBOUND;
        }
        else
        {
            bound = EXACTBOUND;
        }

        transposition_table.storeEntry(hashKey, bound, pv.moves[0], depth, best);
        return alpha;
    }

    /* Iterative Deepening */
    void iterative_deepening(Board& board, int target_depth = 100, bool is_timed = true)
    {
        int alpha = -999999;
        int beta = 999999;
        int lastDepth = 0;
        pv_history.clear();
        std::chrono::microseconds start_time = get_current_time();
        for (int depth = 1; depth <= target_depth; depth++)
        {
            if (this->check_time() && is_timed)
            {
                break;
            }

            if (depth == MAX_DEPTH){
                break;
            }
            
            PV pv;
            int currentScore = this->alpha_beta(board, pv, alpha, beta, depth, 0, true, is_timed);
            pv_history.add_pv(pv, depth);
            if (this->check_time()){
                lastDepth = depth - 1;
            }else{
                lastDepth = depth;
                std::cout << "info depth " << depth;
                std::cout << " score cp " << currentScore;
                std::cout << " time " << (get_current_time() - start_time).count()/1000 << "\n";
            }
        }
        std::cout << "bestmove " << convertMoveToUci(pv_history.pv_list[lastDepth].moves[0]) << "\n";
    }
};
