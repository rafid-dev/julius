#pragma once

#include <algorithm>
#include <chrono>
#include <cmath>
#include "tt.h"
#include "move_ordering.h"
#include "eval.h"

#define BEST_SCORE 999999999
#define MATE_SCORE 9999999
#define DRAW_SCORE 0
#define MAX_DEPTH 64

using namespace Chess;

std::chrono::microseconds get_current_time();

void initSearch();

bool is_check(Board &board, Color side);

class Search
{
private:
    std::chrono::microseconds wtime;
    std::chrono::microseconds btime;
    std::chrono::microseconds winc;
    std::chrono::microseconds binc;
    std::chrono::microseconds lastTime;
    std::chrono::microseconds timePerMove;
    Move killers[MAX_DEPTH][2];
    History history;
    bool stop = false;

public:
Search();
    /*
        clear everything for new game;
    */
    void new_game();
    /*
        Storing killers
    */
    void store_killer(int ply, Move move);
    /*

        Time Management

    */
    void stop_timer();
    void set_timer(std::chrono::microseconds wtime, std::chrono::microseconds btime, std::chrono::microseconds timePerMove, std::chrono::microseconds winc, std::chrono::microseconds binc);
    bool check_time();

    /*
        Qsearch
    */
    int quiesce(Board &board, int alpha, int beta, int ply, int& nodes, bool is_timed);
    /*
        Negamax with alpha beta.
    */
    int alpha_beta(Board &board, int alpha, int beta, int depth, int ply, int& nodes, bool DO_NULL, bool is_timed);
    /* Iterative Deepening */
    void iterative_deepening(Board &board, int target_depth, bool is_timed);
};
