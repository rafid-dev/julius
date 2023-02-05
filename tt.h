#pragma once

#include "chess.hpp"

enum Flag : uint8_t
{
    NONEBOUND,
    UPPERBOUND,
    LOWERBOUND,
    EXACTBOUND
};

struct TTEntry
{
    U64 key = 0;
    int score = 0;
    Chess::Move move = Chess::NO_MOVE;
    uint8_t depth = 0;
    Flag flag = NONEBOUND;
};

class TranspositionTable
{
private:
    std::vector<TTEntry> entries;

public:
    TranspositionTable(int usersize);
    void storeEntry(U64 key, Flag f, Chess::Move move, int depth, int bestScore);
    TTEntry probeEntry(U64 key);
    void clear();
};