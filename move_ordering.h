#include "chess.hpp"

namespace MOVE_ORDER
{
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


    int score_move(Board &board, Move move, Move tt_move = NO_MOVE)
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
            return PROMOTED_SCORE + values[piece(move)];
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

    void give_moves_score(Movelist &movelist, Move ttmove, Board &board)
    {
        for (int i = 0; i < int(movelist.size); i++)
        {
            movelist[i].value = score_move(board, movelist[i].move, ttmove);
        }
    }
}