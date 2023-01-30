#include "chess.hpp"
#include "defs.h"
#include "pesto_tables.h"

using namespace Chess;

int mg_table[12][64];
int eg_table[12][64];
int* mg_pesto_table[6] =
{
    mg_pawn_table,
    mg_knight_table,
    mg_bishop_table,
    mg_rook_table,
    mg_queen_table,
    mg_king_table
};

int* eg_pesto_table[6] =
{
    eg_pawn_table,
    eg_knight_table,
    eg_bishop_table,
    eg_rook_table,
    eg_queen_table,
    eg_king_table
};

void init_tables(){
    for (Piece pc = WhitePawn;pc < BlackPawn; pc++){
        PieceType p = PieceToPieceType[pc];
        for (Square sq = SQ_A1; sq < NO_SQ; sq++) {
            mg_table[pc][sq] = mg_value[p] + mg_pesto_table[p][sq];
            eg_table[pc][sq] = eg_value[p] + eg_pesto_table[p][sq];
            mg_table[pc+6][sq] = mg_value[p] + mg_pesto_table[p][sq^56];
            eg_table[pc+6][sq] = eg_value[p] + eg_pesto_table[p][sq^56];
        }
    }
}

int eval(Board& board){
    int mg[2];
    int eg[2];
    int gamePhase = 0;

    Color side2move = board.sideToMove;
    Color otherSide = (side2move == Black) ? White : Black;

    mg[White] = 0;
    mg[Black] = 0;
    eg[White] = 0;
    eg[Black] = 0;

    U64 white = board.Us(White);
    U64 black = board.Us(Black);

    while (white){
        Square sq = poplsb(white);
        Piece p = board.pieceAtB(sq);
        mg[White] += mg_table[p][sq];
        eg[White] += eg_table[p][sq];
        gamePhase += gamephaseInc[p];

    }

    while (black){
        Square sq = poplsb(black);
        Piece p = board.pieceAtB(sq);
        mg[Black] += mg_table[p][sq];
        eg[Black] += eg_table[p][sq];
        gamePhase += gamephaseInc[p];
    }


    /* tapered eval */
    int mgScore = mg[side2move] - mg[otherSide];
    int egScore = eg[side2move] - eg[otherSide];
    int mgPhase = gamePhase;
    if (mgPhase > 24) mgPhase = 24;
    int egPhase = 24 - mgPhase;

    return (mgScore * mgPhase + egScore * egPhase) / 24;
}