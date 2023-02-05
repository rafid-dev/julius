#pragma once

#include "chess.hpp"

#define TT_SCORE 1000000
#define PROMOTED_SCORE 90000
#define MVVLVA_OFFSET 50000
#define KILLER1 40000
#define KILLER2 35000

int depthBonus(int depth);

struct History
{
    int list[2][64][64];
    void add(Chess::Color side, Chess::Move move, int db)
    {
        list[side][int(from(move))][int(to(move))] += db;
    }
    int index(Chess::Color side, Chess::Move move)
    {
        return list[side][int(from(move))][int(to(move))];
    }
};

struct MoveInfo
{
    Chess::Move tt_move;
    Chess::Move Killers[2];
};

int score_move(Chess::Board &board, Chess::Move move, MoveInfo moveInfo, History &history);
void give_moves_score(Chess::Movelist &movelist, MoveInfo moveInfo, History &history, Chess::Board &board);