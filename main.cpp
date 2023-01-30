#include "chess.hpp"
#include "pesto_tables.h"
#include "defs.h"
#include <iomanip>
#include <sstream>
#include <cstring>
#include <chrono>
#include <algorithm>

using namespace Chess;

void uci_send_id(){
    std::cout << "id name Julius\n";
    std::cout << "id author Slender\n";
    std::cout << "uciok\n";
}

int main()
{
    init_tables();

    //main game loop

    Board board(DEFAULT_POS);
    std::string command;
    std::string token;
    while(true){
        token.clear();

        std::getline(std::cin, command);
        std::istringstream is(command);
        is >> std::skipws >> token;
        if (token == "quit"){
            break;
        }
        else if (token == "isready"){
            std::cout << "readyok\n";

        }else if (token == "uci"){

            uci_send_id();

        }else if (token == "position"){
            std::string option;
            is >> std::skipws >> option;
            if (option == "startpos"){
                board.applyFen(DEFAULT_POS);
            }else if (option == "fen"){
                std::string fen = command.erase(0, 13);
                board.applyFen(fen);
            }
            is >> std::skipws >> option;
            if (option == "moves"){
                std::string moveString;

                while (is >> moveString){
                    std::cout << moveString << std::endl;
                    board.makeMove(convertUciToMove(board, moveString));
                }
            }
        }else if (token == "go"){
            is >> std::skipws >> token;
            if (token == "depth"){
                is >> std::skipws >> token;
                int target_depth = stoi(token);
                int alpha = -100000;
                int beta = 100000;
                PV pv;
                
                for (int depth = 1; depth <= target_depth;depth++){
                    std::cout << "info depth " << depth;
                    int currentScore = alpha_beta(board, pv, alpha, beta, depth);
                    std::cout << " score cp " << currentScore << "\n";
                }
                std::cout << "bestmove " << convertMoveToUci(pv.moves[0]) << std::endl;
            }
            
        }else if (token == "printboard"){ // non uci
            std::cout << board << std::endl;
        }
    }

    return 0;
}