#include "move_ordering.h"

int pcvalues[6] = {100, 300, 300, 500, 900, 0};

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

int depthBonus(int depth)
{
    return std::min(2000, depth * 155);
}

int score_move(Chess::Board &board, Chess::Move move, MoveInfo moveInfo, History &history)
{
    // Piece attacker = board.pieceAtB(from(move));
    Chess::Piece victim = board.pieceAtB(Chess::to(move));
    Chess::Piece attacker = board.pieceAtB(Chess::from(move));
    int score = 0;
    if (move == moveInfo.tt_move)
    {
        return score += TT_SCORE;
    }
    if (Chess::promoted(move))
    {
        return score += PROMOTED_SCORE + pcvalues[piece(move)];
    }
    if (victim != Chess::None)
    {
        return score += MVVLVA_OFFSET + mvv_lva[attacker][victim];
    }
    else if (move == moveInfo.Killers[0])
    {
        score += KILLER1;
    }
    else if (move == moveInfo.Killers[1])
    {
        score += KILLER2;
    }
    else
    {
        //printf("%d\n", history.list[board.sideToMove][from(move)][to(move)]);
        return 0;//return history.list[board.sideToMove][from(move)][to(move)];
    }
    return score;
}

void give_moves_score(Chess::Movelist &movelist, MoveInfo moveInfo, History &history, Chess::Board &board)
{
    for (int i = 0; i < int(movelist.size); i++)
    {
        movelist[i].value = score_move(board, movelist[i].move, moveInfo, history);
    }
}
