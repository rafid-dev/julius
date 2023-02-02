#include <iomanip>
#include <sstream>
#include "search.h"

using namespace Chess;

void uci_send_id()
{
    std::cout << "id name Julius\n";
    std::cout << "id author Slender\n";
    std::cout << "uciok\n";
}

void uci_check_time(std::istringstream &is, std::string &token, int &wtime, int &btime)
{
    if (token == "wtime")
    {
        is >> std::skipws >> token;
        wtime = std::stoi(token);
    }
    else if (token == "btime")
    {
        is >> std::skipws >> token;
        btime = std::stoi(token);
    }
}

void uci_check_time_inc(std::istringstream &is, std::string &token, int &winc, int &binc)
{
    if (token == "winc")
    {
        is >> std::skipws >> token;
        winc = std::stoi(token);
    }
    else if (token == "binc")
    {
        is >> std::skipws >> token;
        binc = std::stoi(token);
    }
}

int main()
{
    init_tables();
    // main game loop
    Board board(DEFAULT_POS);
    Search search(board);
    std::string command;
    std::string token;
    while (true)
    {
        token.clear();

        std::getline(std::cin, command);
        std::istringstream is(command);
        is >> std::skipws >> token;
        if (token == "quit")
        {
            break;
        }
        else if (token == "isready")
        {
            std::cout << "readyok\n";
        }
        else if (token == "uci")
        {

            uci_send_id();
        }
        else if (token == "position")
        {
            std::string option;
            is >> std::skipws >> option;
            if (option == "startpos")
            {
                board.applyFen(DEFAULT_POS);
            }
            else if (option == "fen")
            {
                std::string fen = command.erase(0, 13);
                board.applyFen(fen);
            }
            is >> std::skipws >> option;
            if (option == "moves")
            {
                std::string moveString;

                while (is >> moveString)
                {
                    std::cout << moveString << std::endl;
                    board.makeMove(convertUciToMove(board, moveString));
                }
            }else{
                is >> std::skipws >> option;
                is >> std::skipws >> option;
                is >> std::skipws >> option;
                is >> std::skipws >> option;
                is >> std::skipws >> option;
                is >> std::skipws >> option;
                std::string moveString;

                while (is >> moveString)
                {
                    //std::cout << moveString << std::endl;
                    board.makeMove(convertUciToMove(board, moveString));
                }
            }
        }
        else if (token == "go")
        {
            is >> std::skipws >> token;
            int wtime;
            int btime;
            int winc;
            int binc;
            int moveTime = 0;
            if (token == "depth")
            {
                is >> std::skipws >> token;
                search.set_timer(std::chrono::milliseconds(wtime), std::chrono::milliseconds(btime), std::chrono::milliseconds(INT32_MAX), std::chrono::milliseconds(winc), std::chrono::milliseconds(binc));
                int target_depth = stoi(token);
                search.iterative_deepening(board, target_depth);
            }
            else
            {
                uci_check_time(is, token, wtime, btime);
                is >> std::skipws >> token;
                uci_check_time(is, token, wtime, btime);
                is >> std::skipws >> token;
                uci_check_time_inc(is, token, winc, binc);
                is >> std::skipws >> token;
                uci_check_time_inc(is, token, winc, binc);
                if (moveTime == 0)
                {
                    moveTime = (board.sideToMove == Black) ? (btime / 20 + (binc / 2)) : (wtime / 20 + (binc / 2));
                }
                search.set_timer(std::chrono::milliseconds(wtime), std::chrono::milliseconds(btime), std::chrono::milliseconds(moveTime), std::chrono::milliseconds(winc), std::chrono::milliseconds(binc));
                search.iterative_deepening(board, 100);
            }
            //}
            // is >> std::skipws >> token;
            /*if (token == "movetime")
            {
                is >> std::skipws >> token;
                moveTime = stoi(token);
            }*/
        }
        else if (token == "printboard")
        { // non uci
            std::cout << board << std::endl;
        }
        else if (token == "legalmoves")
        {
            Movelist moveslist;
            Movegen::legalmoves<ALL>(board, moveslist);
            for (int i = 0; i < int(moveslist.size); i++)
            {
                std::cout << convertMoveToUci(moveslist[i].move) << std::endl;
            }
        }
        else if (token == "checktime")
        {
            std::cout << search.check_time() << std::endl;
        }else if (token == "alphabeta"){
            PV pv;
            //search.alpha_beta(board, pv, -100000, 100000, 5, 0);
            std::cout << pv.moves[0] << std::endl;
        }
    }

    return 0;
}