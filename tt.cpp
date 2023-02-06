#include "tt.h"

uint32_t reduce_hash(uint32_t x, uint32_t N)
{
    return ((uint64_t)x * (uint64_t)N) >> 32;
}

TranspositionTable::TranspositionTable(int usersize)
{
    this->entries.resize(usersize / sizeof(TTEntry), TTEntry());
    TTEntry e;
    std::fill(entries.begin(), entries.end(), e);
};

void TranspositionTable::storeEntry(U64 key, Flag f, Chess::Move move, int depth, int bestScore)
{
    int n = this->entries.size();
    int i = reduce_hash(key, n);
    TTEntry entry;
    entry.score = bestScore;
    entry.flag = f;
    entry.depth = depth;
    entry.key = key;
    entry.move = move;
    this->entries[i] = entry;
}

TTEntry TranspositionTable::probeEntry(U64 key)
{
    int n = this->entries.size();
    int i = reduce_hash(key, n);
    return this->entries[i];
}

void TranspositionTable::clear()
{
    TTEntry e;
    std::fill(entries.begin(), entries.end(), e);
}