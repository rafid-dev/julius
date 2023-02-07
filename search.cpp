#include "search.h"
#include "move_ordering.h"

using namespace Chess;

int LMRTable[MAX_DEPTH][MAX_DEPTH];
int LateMovePruningCounts[2][9];

TranspositionTable transposition_table(16 * 1024 * 1024);

// PV length
int pv_length[MAX_DEPTH];

// PV table
Move pv_table[MAX_DEPTH][MAX_DEPTH];

std::chrono::microseconds get_current_time()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch());
}

void initSearch()
{
    init_tables();
    // Init Late Move Reductions Table
    for (int depth = 1; depth < MAX_DEPTH; depth++)
    {
        for (int played = 1; played < MAX_DEPTH; played++)
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

Search::Search()
{
    this->lastTime = get_current_time();
}

void Search::new_game()
{
    transposition_table.clear();
    memset(pv_length, 0, sizeof(pv_length));
    this->killers[MAX_DEPTH][0] = NO_MOVE;
    this->killers[MAX_DEPTH][1] = NO_MOVE;
    history.clear();
}

void Search::store_killer(int ply, Move move)
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

void Search::stop_timer()
{
    this->stop = true;
}

void Search::set_timer(std::chrono::microseconds wtime, std::chrono::microseconds btime, std::chrono::microseconds timePerMove, std::chrono::microseconds winc = std::chrono::microseconds(0), std::chrono::microseconds binc = std::chrono::microseconds(0))
{
    this->wtime = wtime;
    this->btime = btime;
    this->winc = winc;
    this->binc = binc;
    this->timePerMove = timePerMove;
    this->lastTime = get_current_time();
    this->stop = false;
}

bool Search::check_time()
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

int Search::quiesce(Board &board, int alpha, int beta, int ply, int &nodes, bool promoting, bool is_timed = true)
{
    if (this->check_time() && is_timed && nodes & 1023)
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

    // Delta pruning
    /*int BIG_DELTA = 975;
    if (promoting){
        BIG_DELTA += 775;
    }

    if (best < alpha - BIG_DELTA){
        return alpha;
    }*/

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
        nodes++;
        int score = -quiesce(board, -beta, -alpha, ply + 1, nodes, promoted(move), is_timed);
        board.unmakeMove(move);
        if (this->check_time() && is_timed && nodes & 1023)
        {
            return 0;
        }
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

int Search::alpha_beta(Board &board, int alpha, int beta, int depth, int ply, int &nodes, bool DO_NULL, bool is_timed = true)
{
    pv_length[ply] = ply;
    if (board.isRepetition())
    {
        return DRAW_SCORE - ply;
    }
    if (this->check_time() && is_timed && nodes & 1023)
    {
        return 0;
    }
    if (ply > MAX_DEPTH - 1)
    {
        return eval(board);
    }
    if (depth == 0)
    {
        return quiesce(board, alpha, beta, ply, nodes, false, is_timed);
    }

    int best = -BEST_SCORE;
    bool isPVNode = beta - alpha != 1;

    this->killers[MAX_DEPTH][0] = NO_MOVE;
    this->killers[MAX_DEPTH][1] = NO_MOVE;

    int posEval = eval(board);
    if (!is_check(board, board.sideToMove)){
        static_evals[ply] = posEval;
    }

    bool improving = !is_check(board, board.sideToMove) && posEval > (static_evals[ply - 2]);

    if (!isPVNode && !is_check(board, board.sideToMove))
    {
        /*Reverse futility pruning (RFP)*/
        if (depth <= 5 && posEval >= beta && posEval - (depth * 75) >= beta && posEval < MATE_SCORE)
        {
            return posEval;
        }
        /*Null move pruning*/
        if (depth >= 3 && DO_NULL)
        {
            board.makeNullMove();
            best = -this->alpha_beta(board, -beta, -beta + 1, depth - 2, ply + 1, nodes, false, is_timed);
            board.unmakeNullMove();
            if (this->check_time() && is_timed && nodes & 1023)
            {

                return 0;
            }
            if (best >= beta)
            {
                if (best >= MATE_SCORE)
                {
                    return beta;
                }
                return best;
            }
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
    give_moves_score(moveslist, move_info, history, board);
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
        nodes++;
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

        if (depth > 3 && i >= 5 && !is_capture && !promoted(move))
        {
            int lmrDepth = LMRTable[std::min(depth, 64)][std::min(i, 63)];
            // increase for non pv nodes
            lmrDepth += !isPVNode;
            // increase for king moves that evade check
            lmrDepth += is_check(board, board.sideToMove) && board.pieceTypeAtB(to(move)) == KING;
            lmrDepth = std::max(1, lmrDepth);
            //lmrDepth -= (move == killers[ply][0]) || (move == killers[ply][1]);
            score = -alpha_beta(board, -alpha - 1, -alpha, depth-lmrDepth, ply + 1, nodes, DO_NULL, is_timed);
            do_full_search = score > alpha && lmrDepth != 1;
        }
        else
        {
            do_full_search = !isPVNode || i > 0;
        }

        /*
        Search with reduced window but full depth.
        */
        if (do_full_search)
        {
            score = -alpha_beta(board, -alpha - 1, -alpha, depth - 1, ply + 1, nodes, DO_NULL, is_timed);
        }
        /*
            Principal variation search (PVS).
        */
        if (isPVNode && ((score > alpha && score < beta) || i == 0))
        {
            score = -alpha_beta(board, -beta, -alpha, depth - 1, ply + 1, nodes, DO_NULL, is_timed);
        }

        board.unmakeMove(move);
        if (this->check_time() && is_timed && nodes & 1024)
        {

            return 0;
        }
        if (score > best)
        {
            best = score;
            if (score > alpha)
            {
                alpha = score;

                // write PV move
                pv_table[ply][ply] = move;

                // loop over the next ply
                for (int next_ply = ply + 1; next_ply < pv_length[ply + 1]; next_ply++)
                    // copy move from deeper ply into a current ply's line
                    pv_table[ply][next_ply] = pv_table[ply + 1][next_ply];

                // adjust PV length
                pv_length[ply] = pv_length[ply + 1];

                if (score >= beta)
                {
                    if (!is_capture)
                    {
                        store_killer(ply, move);
                        history.add(board.sideToMove, move, depth*depth);
                    }
                    break;
                }
            }
        }
        //no beta cutoff so we substract from history
        history.add(board.sideToMove, move, -(depth*depth));
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

    transposition_table.storeEntry(hashKey, bound, pv_table[0][0], depth, best);
    return alpha;
}

void Search::iterative_deepening(Board &board, int target_depth = 100, bool is_timed = true)
{
    int alpha = -999999;
    int beta = 999999;
    int lastDepth = 0;
    std::chrono::microseconds start_time = get_current_time();
    int nodes = 0;
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
        int currentScore = alpha_beta(board, alpha, beta, depth, 0, nodes, true, is_timed);
        lastDepth = (this->check_time() && is_timed) ? depth - 1 : depth;
        std::cout << "info depth " << depth;
        std::cout << " score cp " << currentScore;
        std::cout << " time " << (get_current_time() - start_time).count() / 1000 << " nodes " << nodes << " pv ";

        // loop over the moves within a PV line
        for (int i = 0; i < pv_length[0]; i++)
        {
            // print PV move
            std::cout << convertMoveToUci(pv_table[0][i]) << " ";
        }
        std::cout << "\n";
    }
    // pv_history.pv_list[lastDepth].print();
    std::cout << "bestmove " << convertMoveToUci(pv_table[0][0]) << "\n";
    history.clear();
}