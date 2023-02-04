#include <iomanip>
#include <sstream>
#include <thread>
#include "search.h"

using namespace Chess;

void uci_send_id()
{
    std::cout << "id name Julius v1.5.0\n";
    std::cout << "id author Slender\n";
    std::cout << "uciok\n";
}

void uci_check_time(std::istringstream &is, std::string &token, int64_t &wtime, int64_t &btime)
{
    if (token == "wtime")
    {
        is >> std::skipws >> token;
        wtime = std::stoi(token) * 1000; // convert to microseconds
    }
    else if (token == "btime")
    {
        is >> std::skipws >> token;
        btime = std::stoi(token) * 1000;
    }
}

void uci_check_time_inc(std::istringstream &is, std::string &token, int64_t &winc, int64_t &binc)
{
    if (token == "winc")
    {
        is >> std::skipws >> token;
        winc = std::stoi(token) * 1000; // convert to microseconds
    }
    else if (token == "binc")
    {
        is >> std::skipws >> token;
        binc = std::stoi(token) * 1000;
    }
}

/*void start_uci()
{
}*/

/*void start_search(Search& search, Board& board, int max_depth, bool timed)
{
    search.iterative_deepening(board, max_depth, timed);
}*/

int main()
{
    init_tables();
    // main game loop
    Search search;
    Board board(DEFAULT_POS);
    std::string command;
    std::string token;
    
    while (true)
    {
        std::cout.flush();
        token.clear();

        std::getline(std::cin, command);
        std::istringstream is(command);
        is >> std::skipws >> token;
        if (token == "stop")
        {
            search.stop_timer();
            continue;
        }
        if (token == "quit")
        {
            break;
        }
        if (token == "isready" || token == "ucinewgame")
        {
            std::cout << "readyok\n";
            continue;
        }
        if (token == "uci")
        {
            uci_send_id();
            continue;
        }
        if (token == "position")
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
                    // std::cout << moveString << std::endl;
                    board.makeMove(convertUciToMove(board, moveString));
                }
            }
            else
            {
                is >> std::skipws >> option;
                is >> std::skipws >> option;
                is >> std::skipws >> option;
                is >> std::skipws >> option;
                is >> std::skipws >> option;
                is >> std::skipws >> option;
                std::string moveString;

                while (is >> moveString)
                {
                    // std::cout << moveString << std::endl;
                    board.makeMove(convertUciToMove(board, moveString));
                }
            }
            continue;
        }
        if (token == "go")
        {
            is >> std::skipws >> token;
            int64_t wtime = 100000;
            int64_t btime = 100000;
            int64_t winc = 1000;
            int64_t binc = 1000;
            int64_t moveTime = 100 * 1000;
            if (token == "depth")
            {
                is >> std::skipws >> token;
                search.set_timer(std::chrono::microseconds(wtime), std::chrono::milliseconds(btime), std::chrono::milliseconds(INT32_MAX), std::chrono::milliseconds(winc), std::chrono::milliseconds(binc));
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
                moveTime = (board.sideToMove == Black) ? (btime / 20) + (binc / 2) : (wtime / 20) + (winc / 2);
                search.set_timer(std::chrono::microseconds(wtime), std::chrono::microseconds(btime), std::chrono::microseconds(moveTime), std::chrono::microseconds(winc), std::chrono::microseconds(binc));
                search.iterative_deepening(board, MAX_DEPTH);
            }
            //}
            // is >> std::skipws >> token;
            /*if (token == "movetime")
            {
                is >> std::skipws >> token;
                moveTime = stoi(token);
            }*/
            continue;
        }
        if (token == "printboard")
        { // non uci
            std::cout << board << std::endl;
            continue;
        }
        if (token == "legalmoves")
        {
            Movelist moveslist;
            Movegen::legalmoves<ALL>(board, moveslist);
            for (int i = 0; i < int(moveslist.size); i++)
            {
                std::cout << convertMoveToUci(moveslist[i].move) << std::endl;
            }
            continue;
        }
    }

    return 0;
}