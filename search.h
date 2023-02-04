#pragma once

#include <cstring>
#include <algorithm>
#include <chrono>
#include "eval.h"
#include "tt.h"

using namespace Chess;

int mvv_lva[12][12] = {
 	105, 205, 305, 405, 505, 605,  105, 205, 305, 405, 505, 605,
	104, 204, 304, 404, 504, 604,  104, 204, 304, 404, 504, 604,
	103, 203, 303, 403, 503, 603,  103, 203, 303, 403, 503, 603,
	102, 202, 302, 402, 502, 602,  102, 202, 302, 402, 502, 602,
	101, 201, 301, 401, 501, 601,  101, 201, 301, 401, 501, 601,
	100, 200, 300, 400, 500, 600,  100, 200, 300, 400, 500, 600,

	105, 205, 305, 405, 505, 605,  105, 205, 305, 405, 505, 605,
	104, 204, 304, 404, 504, 604,  104, 204, 304, 404, 504, 604,
	103, 203, 303, 403, 503, 603,  103, 203, 303, 403, 503, 603,
	102, 202, 302, 402, 502, 602,  102, 202, 302, 402, 502, 602,
	101, 201, 301, 401, 501, 601,  101, 201, 301, 401, 501, 601,
	100, 200, 300, 400, 500, 600,  100, 200, 300, 400, 500, 600
};

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
    PV pv_list[256];
    void add_pv(PV& pv, int depth){
        pv_list[depth] = pv;
        length += 1;
    }
    void clear(){
        for (int i = 0; i < length;i++){
            pv_list[i] = PV();
        }
        length = 0;
    }
};

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

    int alpha_beta(Board& board, PV &pv, int alpha, int beta, int depth, int ply, bool DO_NULL, bool is_timed = true)
    {
        if (board.isRepetition() || (this->check_time() && is_timed)){
            return 0;
        }
        int best = -BEST_SCORE;

        if (ply > MAX_DEPTH - 1)
        {
            return eval(board);
        }
        if (depth == 0)
        {
            return this->quiesce(board, alpha, beta, ply, is_timed);
        }

        PV local_pv;
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
            int score = -this->alpha_beta(board, local_pv, -beta, -alpha, depth - 1, ply + 1, DO_NULL, is_timed);
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

    void iterative_deepening(Board& board, int target_depth = 100, bool is_timed = true)
    {
        int alpha = -100000;
        int beta = 100000;
        int lastDepth = 0;
        pv_history.clear();
        std::chrono::microseconds start_time = get_current_time();
        for (int depth = 1; depth <= target_depth; depth++)
        {
            if (this->check_time() && is_timed)
            {
                // std::cout << "TIME UP AT DEPTH " << depth << std::endl;
                //  std::cout << "breaking" << std::endl;
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
        /*for (int i = 0; i < target_depth; i++){
            PV pv = pv_history.pv_list[i];
            std::cout << pv.length << std::endl;
            for (int i2 = 0; i2 < pv.length; i2++){
                std::cout << "depth: " << i << " ";
                std::cout << convertMoveToUci(pv.moves[i2]) << "\n";
            }
        }*/
    }

    int score_move(Board& board, Move move, Move tt_move = NO_MOVE)
    {
        // Piece attacker = board.pieceAtB(from(move));
        Piece victim = board.pieceAtB(to(move));
        Piece attacker = board.pieceAtB(from(move));
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
            return mvv_lva[attacker][victim];
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
