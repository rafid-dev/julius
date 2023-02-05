#pragma once

#include <cstring>
#include "chess.hpp"

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