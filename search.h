#pragma once

#include <cstring>
#include <algorithm>
#include <chrono>
#include "eval.h"
#include "tt.h"

using namespace Chess;

struct PV
{
    int length = 0;
    int score;
    Chess::Move moves[256];

    void load_from(Chess::Move m, const PV &rest)
    {
        moves[0] = m;
        std::memcpy(moves + 1, rest.moves, sizeof(m) * rest.length);
        length = rest.length + 1;
    }
};

struct PVHistory
{
    int length = 0;
    PV pv_list[MAX_DEPTH];
    void add_pv(PV& pv, int depth){
        pv_list[depth] = pv;
        length += 1;
    }
    void clear(){
        for (int i = 0; i < length;i++){
            pv_list[i] = PV();
        }
    }
};

std::chrono::milliseconds get_current_time()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
}

TranspositionTable transposition_table(16 * 1024 * 1024);
PVHistory pv_history;

class Search
{
private:
    std::chrono::milliseconds wtime;
    std::chrono::milliseconds btime;
    std::chrono::milliseconds winc;
    std::chrono::milliseconds binc;
    std::chrono::milliseconds lastTime = get_current_time();
    std::chrono::milliseconds timePerMove;
    bool stop = false;

public:
    void set_timer(std::chrono::milliseconds wtime, std::chrono::milliseconds btime, std::chrono::milliseconds timePerMove, std::chrono::milliseconds winc = std::chrono::milliseconds(0), std::chrono::milliseconds binc = std::chrono::milliseconds(0))
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
    int quiesce(Board& board, int alpha, int beta, int ply)
    {

        if (ply >= MAX_PLY)
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
            int score = -this->quiesce(board, -beta, -alpha, ply + 1);
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

    int alpha_beta(Board& board, PV &pv, int alpha, int beta, int depth, int ply, bool &finishedDepth)
    {
        if (depth > 2 && this->check_time() || this->stop)
        {
            return 0;
        }
        int best = -BEST_SCORE;

        if (ply >= MAX_PLY)
        {   
            return eval(board);
        }
        if (ply > MAX_DEPTH - 1)
        {
            return eval(board);
        }
        if (depth == 0)
        {
            return this->quiesce(board, alpha, beta, ply);
        }

        int oldAlpha = alpha;
        PV local_pv;
        Movelist moveslist;
        Movegen::legalmoves<ALL>(board, moveslist);

        U64 hashKey = board.hashKey;
        TTEntry tte = transposition_table.probeEntry(hashKey);

        this->give_moves_score(moveslist, tte.move, board);
        std::sort(moveslist.list, moveslist.list + moveslist.size, [](ExtMove a, ExtMove b)
                  { return (a.value > b.value); });

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

        if (moveslist.size == 0)
        {
            if (board.isSquareAttacked(~board.sideToMove, board.KingSQ(board.sideToMove)))
            {
                return -MATE_SCORE + ply;
            }
            return DRAW_SCORE;
        }

        for (int i = 0; i < int(moveslist.size); i++)
        {
            Move move = moveslist[i].move;
            // std::cout << convertMoveToUci(move) << std::endl;
            board.makeMove(move);
            int score = -this->alpha_beta(board, local_pv, -beta, -alpha, depth - 1, ply + 1, finishedDepth);
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
        if (this->check_time() || this->stop)
        {
            finishedDepth = false;
        }
        else
        {
            finishedDepth = true;
        }
        return alpha;
    }

    void iterative_deepening(Board& board, int target_depth = 100)
    {
        int alpha = -100000;
        int beta = 100000;
        int lastDepth;
        pv_history.clear();
        std::chrono::milliseconds start_time = get_current_time();
        for (int depth = 1; depth <= target_depth; depth++)
        {
            if (depth > 2 && ((this->check_time()) || (this->stop)))
            {
                // std::cout << "TIME UP AT DEPTH " << depth << std::endl;
                //  std::cout << "breaking" << std::endl;
                break;
            }
            if (depth == MAX_DEPTH){
                break;
            }
            PV pv;
            bool finishedDepth = false;
            int currentScore = this->alpha_beta(board, pv, alpha, beta, depth, 0, finishedDepth);
            pv_history.add_pv(pv, depth);
            if (finishedDepth == false)
            {
                lastDepth = depth - 1;
            }
            else
            {
                std::cout << "info depth " << depth;
                std::cout << " score cp " << currentScore;
                std::cout << " time " << (get_current_time() - start_time).count() << "\n";
            }
        }
        std::cout << "bestmove " << convertMoveToUci(pv_history.pv_list[lastDepth].moves[0]) << std::endl;
        for (int i = 0; i < target_depth; i++){
            PV pv = pv_history.pv_list[i];
            std::cout << pv.length << std::endl;
            for (int i2 = 0; i2 < pv.length; i2++){
                std::cout << "depth: " << i << " ";
                std::cout << convertMoveToUci(pv.moves[i2]) << "\n";
            }
        }
    }

    int score_move(Board& board, Move move, Move tt_move = NO_MOVE)
    {
        // Piece attacker = board.pieceAtB(from(move));
        Piece victim = board.pieceAtB(to(move));
        if (move == tt_move)
        {
            return TT_SCORE;
        }
        if (promoted(move))
        {
            return PROMOTED_SCORE;
        }
        if (victim != None)
        {
            return CAPTURE_SCORE - values[victim];
        }
        else
        {
            return 0;
        }
    }

    void give_moves_score(Movelist& movelist, Move ttmove, Board& board)
    {
        for (int i = 0; i < int(movelist.size); i++)
        {
            movelist[i].value = this->score_move(board, movelist[i].move, ttmove);
        }
    }
};
