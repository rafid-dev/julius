#pragma once

#include <algorithm>
#include <chrono>
#include <cmath>
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

int LMRTable[64][64];
int LateMovePruningCounts[2][9];

void initSearch()
{

    // Init Late Move Reductions Table
    for (int depth = 1; depth < 64; depth++)
    {
        for (int played = 1; played < 64; played++)
        {
            LMRTable[depth][played] = 0.75 + log(depth) * log(played) / 2.25;
        }
    }

    for (int depth = 1; depth < 9; depth++)
    {
        LateMovePruningCounts[0][depth] = 2.5 + 2 * depth * depth / 4.5;
        LateMovePruningCounts[1][depth] = 4.0 + 4 * depth * depth / 4.5;
    }
}

bool is_check(Board &board, Color side)
{
    return board.isSquareAttacked(~side, board.KingSQ(side));
}

class Search
{
private:
    std::chrono::microseconds wtime;
    std::chrono::microseconds btime;
    std::chrono::microseconds winc;
    std::chrono::microseconds binc;
    std::chrono::microseconds lastTime = get_current_time();
    std::chrono::microseconds timePerMove;
    Move killers[MAX_DEPTH][2];
    History history;
    bool stop = false;

public:
    /*
        clear everything for new game;
    */
    void new_game()
    {
        transposition_table.clear();
        this->killers[MAX_DEPTH][0] = NO_MOVE;
        this->killers[MAX_DEPTH][1] = NO_MOVE;
        pv_history.clear();
    }
    /*
        Storing killers
    */
    void store_killer(int ply, Move move)
    {
        if (this->killers[ply][0] == NO_MOVE)
        {
            this->killers[ply][0] = move;
        }
        else if (this->killers[ply][0] != move)
        {
            this->killers[ply][1] = this->killers[ply][0];
            this->killers[ply][0] = move;
        }
    }
    /*

        Time Management

    */
    void stop_timer()
    {
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
    int quiesce(Board &board, int alpha, int beta, int ply, bool is_timed = true)
    {
        if (this->check_time() && is_timed)
        {
            return 0;
        }

        if (ply >= MAX_DEPTH)
        {
            return eval(board);
        }
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
            int score = -quiesce(board, -beta, -alpha, ply + 1, is_timed);
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
    int alpha_beta(Board &board, PV &pv, int alpha, int beta, int depth, int ply, bool DO_NULL, bool is_timed = true)
    {
        if (board.isRepetition())
        {
            return DRAW_SCORE - ply;
        }
        if (this->check_time() && is_timed)
        {
            return 0;
        }
        if (ply > MAX_DEPTH - 1)
        {
            return eval(board);
        }
        if (depth == 0)
        {
            return quiesce(board, alpha, beta, ply, is_timed);
        }

        int best = -BEST_SCORE;
        bool isPVNode = beta - alpha != 1;
        PV local_pv;
        this->killers[MAX_DEPTH][0] = NO_MOVE;
        this->killers[MAX_DEPTH][1] = NO_MOVE;

        /*
        Null Move pruning
        */
        if (depth >= 3 && DO_NULL && !is_check(board, board.sideToMove))
        {
            board.makeNullMove();
            best = -this->alpha_beta(board, local_pv, -beta, -beta + 1, depth - 2, ply + 1, false, is_timed);
            board.unmakeNullMove();
            if (this->check_time() && is_timed)
            {
                return 0;
            }
            if (best >= beta)
            {
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
            if (is_check(board, board.sideToMove))
            {
                return -MATE_SCORE + ply;
            }
            return DRAW_SCORE;
        }

        // TT entry
        U64 hashKey = board.hashKey;
        TTEntry tte = transposition_table.probeEntry(hashKey);

        // Move scoring
        MoveInfo move_info;
        move_info.tt_move = tte.move;
        move_info.Killers[0] = killers[ply][0];
        move_info.Killers[1] = killers[ply][1];
        int depth_bonus = depthBonus(depth);

        MOVE_ORDER::give_moves_score(moveslist, move_info, history, board);
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
            bool is_capture = board.pieceAtB(to(move)) != None;
            board.makeMove(move);
            int score;

            /*
                Late move reduction (LMR): reduce moves that we think are bad

                Conditions are:
                depth > 3
                move number >= 4
                no checks
                no capture move or promotion move
                not pv node
            */

            bool do_full_search = false;
            //if (depth > 3 && i >= 4 && !is_check(board, board.sideToMove) && !is_capture && !promoted(move))
            //{
            //    int lmrDepth = LMRTable[depth][i + 1];
            //    lmrDepth = std::clamp(depth - lmrDepth, 1, depth + 1);
            //    score = -alpha_beta(board, local_pv, -alpha - 1, -alpha, lmrDepth, ply + 1, DO_NULL, is_timed);
            ///   do_full_search = score >= alpha && lmrDepth != 1;
            //}
            //else
            //{
                do_full_search = !isPVNode || i > 0;
            //}

            /*
            Search with reduced window but full depth.
            */
            if (do_full_search)
            {
                score = -alpha_beta(board, local_pv, -alpha - 1, -alpha, depth - 1, ply + 1, DO_NULL, is_timed);
            }
            /*
                Principal variation search (PVS).
            */
            if (isPVNode && ((score > alpha && score < beta) || i == 0))
            {
                score = -alpha_beta(board, local_pv, -beta, -alpha, depth - 1, ply + 1, DO_NULL, is_timed);
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
                        if (!is_capture)
                        {
                            store_killer(ply, move);
                            history.add(board.sideToMove, move, depth_bonus);
                        }
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
    void iterative_deepening(Board &board, int target_depth = 100, bool is_timed = true)
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
            this->killers[MAX_DEPTH][0] = NO_MOVE;
            this->killers[MAX_DEPTH][1] = NO_MOVE;
            if (depth == MAX_DEPTH)
            {
                break;
            }

            PV pv;
            int currentScore = alpha_beta(board, pv, alpha, beta, depth, 0, true, is_timed);
            pv_history.add_pv(pv, depth);
            if (this->check_time())
            {
                lastDepth = depth - 1;
            }
            else
            {
                lastDepth = depth;
                std::cout << "info depth " << depth;
                std::cout << " score cp " << currentScore;
                std::cout << " time " << (get_current_time() - start_time).count() / 1000 << "\n";
            }
        }
        std::cout << "bestmove " << convertMoveToUci(pv_history.pv_list[lastDepth].moves[0]) << "\n";
    }
};
