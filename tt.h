#include "chess.hpp"

using namespace Chess;

enum Flag : uint8_t{
    NONEBOUND,
    UPPERBOUND,
    LOWERBOUND,
    EXACTBOUND
};

struct TTEntry {
    U64 key = 0;
    int score = 0;
    Move move = NO_MOVE;
    uint8_t depth = 0;
    Flag flag = NONEBOUND;
};

class TranspositionTable{
    private:
    std::vector<TTEntry> entries;

    public:
    TranspositionTable(int usersize){
        this->entries.resize(usersize/sizeof(TTEntry), TTEntry());
        TTEntry e;
        std::fill(entries.begin(), entries.end(), e);
    };

    void storeEntry (U64 key,Flag f, Move move, int depth, int bestScore){
        int n = this->entries.size();
        int i = key%n;
        TTEntry entry;
        entry.score = bestScore;
        entry.flag = f;
        entry.depth = depth;
        entry.key = key;
        entry.move = move;
        this->entries[i] = entry;
    }

    TTEntry probeEntry (U64 key){
        int n = this->entries.size();
        int i = key%n;
        return this->entries[i];
    }
};