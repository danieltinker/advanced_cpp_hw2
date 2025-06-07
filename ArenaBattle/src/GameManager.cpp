#include "GameManager.h"
#include "Board.h"

#include <fstream>
#include <iostream>

using namespace arena;

GameManager::GameManager(std::unique_ptr<common::PlayerFactory>        pFac,
                         std::unique_ptr<common::TankAlgorithmFactory> tFac)
  : game_state_(std::move(pFac), std::move(tFac))
{}

void GameManager::readBoard(const std::string& map_file) {
    loaded_map_file_ = map_file;

    std::ifstream in(map_file);
    if (!in) {
        std::cerr << "Failed to open map file: " << map_file << "\n";
        std::exit(1);
    }

    size_t rows = 0, cols = 0, maxSteps = 0, numShells = 0;
    std::vector<std::string> gridLines;
    std::string line;

    // Parse header
    while (std::getline(in, line)) {
        if (line.rfind("Rows", 0) == 0) {
            rows = std::stoul(line.substr(line.find('=') + 1));
        } else if (line.rfind("Cols", 0) == 0) {
            cols = std::stoul(line.substr(line.find('=') + 1));
        } else if (line.rfind("MaxSteps", 0) == 0) {
            maxSteps = std::stoul(line.substr(line.find('=') + 1));
        } else if (line.rfind("NumShells", 0) == 0) {
            numShells = std::stoul(line.substr(line.find('=') + 1));
        } else {
            // first grid line?
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
        }
    }
    // Read remaining grid rows
    while (gridLines.size() < rows && std::getline(in, line)) {
        if (line.empty()) continue;
        bool isGrid = true;
        for (char ch : line) {
            if (ch != '.' && ch != '#' && ch != '@' && ch != '1' && ch != '2') {
                isGrid = false;
                break;
            }
        }
        if (isGrid) gridLines.push_back(line);
    }

    // Build board
    Board board(rows, cols);
    for (size_t r = 0; r < rows; ++r) {
        const auto& rowStr = gridLines[r];
        for (size_t c = 0; c < cols && c < rowStr.size(); ++c) {
            char ch = rowStr[c];
            switch (ch) {
                case '#': board.setCell(c, r, CellContent::WALL);  break;
                case '@': board.setCell(c, r, CellContent::MINE);  break;
                case '1': board.setCell(c, r, CellContent::TANK1); break;
                case '2': board.setCell(c, r, CellContent::TANK2); break;
                default:  board.setCell(c, r, CellContent::EMPTY);break;
            }
        }
    }

    // Initialize the game
    game_state_.initialize(board, maxSteps, numShells);
}

void GameManager::run() {
    size_t turn = 0;
    while (!game_state_.isGameOver()) {
        std::cout << "=== Turn " << turn << " ===\n";
        game_state_.printBoard();
        std::cout << game_state_.advanceOneTurn() << "\n";
        ++turn;
    }
    std::cout << "=== Final Board ===\n";
    game_state_.printBoard();
    std::cout << game_state_.getResultString() << "\n";
}
