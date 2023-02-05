#include "chess.hpp"

#define TT_SCORE 1000000
#define PROMOTED_SCORE 90000
#define MVVLVA_OFFSET 50000
#define KILLER1 40000
#define KILLER2 35000

int depthBonus (int depth){
    return std::min(2000, depth*155);
}

struct History {
    int list[2][64][64];
    void add (Color side, Move move, int db){
        list[side][int(from(move))][int(to(move))] += db;
    }
    int index (Color side, Move move){
        return list[side][int(from(move))][int(to(move))];
    }
};

struct MoveInfo
{
    Move tt_move;
    Move Killers[2];
};

namespace MOVE_ORDER
{
    int mvv_lva[12][12] = {
        105, 205, 305, 405, 505, 605, 105, 205, 305, 405, 505, 605,
        104, 204, 304, 404, 504, 604, 104, 204, 304, 404, 504, 604,
        103, 203, 303, 403, 503, 603, 103, 203, 303, 403, 503, 603,
        102, 202, 302, 402, 502, 602, 102, 202, 302, 402, 502, 602,
        101, 201, 301, 401, 501, 601, 101, 201, 301, 401, 501, 601,
        100, 200, 300, 400, 500, 600, 100, 200, 300, 400, 500, 600,

        105, 205, 305, 405, 505, 605, 105, 205, 305, 405, 505, 605,
        104, 204, 304, 404, 504, 604, 104, 204, 304, 404, 504, 604,
        103, 203, 303, 403, 503, 603, 103, 203, 303, 403, 503, 603,
        102, 202, 302, 402, 502, 602, 102, 202, 302, 402, 502, 602,
        101, 201, 301, 401, 501, 601, 101, 201, 301, 401, 501, 601,
        100, 200, 300, 400, 500, 600, 100, 200, 300, 400, 500, 600};

    int score_move(Board &board, Move move, MoveInfo moveInfo, History& history)
    {
        // Piece attacker = board.pieceAtB(from(move));
        Piece victim = board.pieceAtB(to(move));
        Piece attacker = board.pieceAtB(from(move));
        if (move == moveInfo.tt_move)
        {
            return TT_SCORE;
        }
        if (promoted(move))
        {
            return PROMOTED_SCORE + values[piece(move)];
        }
        if (victim != None)
        {
            return MVVLVA_OFFSET + mvv_lva[attacker][victim];
        }
        else if (move == moveInfo.Killers[0])
        {
            return KILLER1;
        }
        else if (move == moveInfo.Killers[1])
        {
            return KILLER2;
        }
        else
        {
            return 0;//history.index(board.sideToMove, move);
        }
    }
    void give_moves_score(Movelist &movelist, MoveInfo moveInfo, History& history, Board &board)
    {
        for (int i = 0; i < int(movelist.size); i++)
        {
            movelist[i].value = score_move(board, movelist[i].move, moveInfo, history);
        }
    }
}
