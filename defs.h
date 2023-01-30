#include "chess.hpp"

#define MATE_SCORE 99999999
#define DRAW_SCORE 0

struct PV {
    int length = 0;
    int score;
    Chess::Move moves[256];

    void load_from(Chess::Move m, const PV& rest){
        moves[0] = m;
        std::memcpy(moves + 1, rest.moves, sizeof(m) * rest.length);
        length = rest.length + 1;
    }
};

extern int quiesce(Chess::Board& board, int alpha, int beta);
extern int alpha_beta(Chess::Board&, PV& pv, int alpha, int beta, int depth);
extern int eval(Chess::Board&);
void init_tables();