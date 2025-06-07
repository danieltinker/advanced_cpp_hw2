#include "GameManager.h"
#include "Board.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <string>

using namespace arena;

void GameManager::readBoard(const std::string &map_file) {
    loaded_map_file_ = map_file;

    std::ifstream in(map_file);
    if (!in) {
        std::cerr << "Failed to open map file: " << map_file << std::endl;
        std::exit(1);
    }

    std::string line;
    size_t rows = 0, cols = 0, maxSteps = 0, numShells = 0;
    std::vector<std::string> gridLines;

    // Parse header lines
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        if (line.rfind("Rows", 0) == 0) {
            rows = std::stoul(line.substr(line.find('=') + 1));
        } else if (line.rfind("Cols", 0) == 0) {
            cols = std::stoul(line.substr(line.find('=') + 1));
        } else if (line.rfind("MaxSteps", 0) == 0) {
            maxSteps = std::stoul(line.substr(line.find('=') + 1));
        } else if (line.rfind("NumShells", 0) == 0) {
            numShells = std::stoul(line.substr(line.find('=') + 1));
        } else {
            // Possibly a grid line: only ., #, @, 1, 2
            bool isGrid = !line.empty();
            for (char ch : line) {
                if (ch != '.' && ch != '#' && ch != '@' && ch != '1' && ch != '2') {
                    isGrid = false;
                    break;
                }
            }
            if (isGrid) {
                gridLines.push_back(line);
                break;
            }
            // Otherwise skip non-header title or comments
        }
    }
    // Read remaining grid lines
    while (gridLines.size() < rows && std::getline(in, line)) {
        if (line.empty()) continue;
        // Ensure it's a valid grid line
        bool isGrid = true;
        for (char ch : line) {
            if (ch != '.' && ch != '#' && ch != '@' && ch != '1' && ch != '2') {
                isGrid = false;
                break;
            }
        }
        if (!isGrid) continue;
        gridLines.push_back(line);
    }
    while (gridLines.size() < rows && std::getline(in, line)) {
        if (line.empty()) continue;
        gridLines.push_back(line);
    }

    // Build the board
    Board board(rows, cols);
    for (size_t r = 0; r < rows; ++r) {
        const auto &rowStr = gridLines[r];
        for (size_t c = 0; c < cols && c < rowStr.size(); ++c) {
            char ch = rowStr[c];
            switch (ch) {
                case '#': board.setCell(c, r, CellContent::WALL); break;
                case '@': board.setCell(c, r, CellContent::MINE); break;
                case '1': board.setCell(c, r, CellContent::TANK1); break;
                case '2': board.setCell(c, r, CellContent::TANK2); break;
                default:  board.setCell(c, r, CellContent::EMPTY); break;
            }
        }
    }

    // Initialize game state
    game_state_.initialize(board, maxSteps, numShells);
}

void GameManager::run() {
    size_t turn = 0;
    // Game loop
    while (!game_state_.isGameOver()) {
        std::cout << "=== Turn " << turn << " ===" << std::endl;
        game_state_.printBoard();

        std::string actions = game_state_.advanceOneTurn();
        std::cout << actions << "" << std::endl;
        ++turn;
    }

    // Final board and result
    std::cout << "=== Final Board ===";
    game_state_.printBoard();
    std::cout << game_state_.getResultString() << std::endl;
}