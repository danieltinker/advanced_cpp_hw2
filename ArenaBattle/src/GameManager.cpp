#include "GameManager.h"
#include "Board.h"
#include "utils.h"

#include <fstream>
#include <iostream>

using namespace arena;
using namespace common;

// GameManager::GameManager(std::unique_ptr<PlayerFactory> pFac,
//                          std::unique_ptr<TankAlgorithmFactory> tFac)
//   : game_state_(std::move(pFac), std::move(tFac))
// {}

GameManager::~GameManager() = default;

void GameManager::readBoard(const std::string& map_file) {
    loaded_map_file_ = map_file;

    // (1) Open the map file and parse MaxSteps, NumShells, Rows, Cols:
    std::ifstream in(map_file);
    if (!in) {
        std::cerr << "Cannot open map file: " << map_file << "\n";
        std::exit(1);
    }

    std::string line;
    // map name (ignore)
    std::getline(in, line);

    // MaxSteps = <NUM>
    std::getline(in, line);
    std::size_t maxSteps = 0;
    parseKeyValue(line, "MaxSteps", maxSteps);

    // NumShells = <NUM>
    std::getline(in, line);
    std::size_t numShells = 0;
    parseKeyValue(line, "NumShells", numShells);

    // Rows = <NUM>
    std::getline(in, line);
    std::size_t rows = 0;
    parseKeyValue(line, "Rows", rows);

    // Cols = <NUM>
    std::getline(in, line);
    std::size_t cols = 0;
    parseKeyValue(line, "Cols", cols);

    // (2) Build a Board(rows×cols) and fill cells:
    Board board(rows, cols);
    std::size_t r = 0;
    while (r < rows && std::getline(in, line)) {
        for (std::size_t c = 0; c < cols && c < line.size(); ++c) {
            char ch = line[c];
            CellContent content = CellContent::EMPTY;
            if (ch == '#') {
                content = CellContent::WALL;
            } else if (ch == '@') {
                content = CellContent::MINE;
            } else if (ch == '1') {
                content = CellContent::TANK1;
            } else if (ch == '2') {
                content = CellContent::TANK2;
            } else {
                content = CellContent::EMPTY;
            }
            board.setCell(static_cast<int>(c), static_cast<int>(r), content);
        }
        ++r;
    }
    in.close();

    // (3) Initialize GameState with board, maxSteps, numShells:
    game_state_.initialize(board, maxSteps, numShells);
}


void GameManager::run() {
    // Output file is named "output_<loaded_map_file_>"
    std::string out_filename = "output_" + loaded_map_file_;
    std::ofstream out(out_filename);
    if (!out) {
        std::cerr << "Cannot open output file: " << out_filename << "\n";
        return;
    }

    int turn = 0;
    while (!game_state_.isGameOver()) {
        // --- Print to console ---
        std::cout << "=== Turn " << turn << " ===\n";
        game_state_.printBoard();
        std::cout << "\n";
        // Advance one turn in the game state; get back a comma-separated list
        // of what each tank did this turn (with "(ignored)" or "(killed)" tags as needed).
        std::string turnStr = game_state_.advanceOneTurn();
        
        // Show that line of actions on-screen
        std::cout << turnStr << "\n\n";

        // --- Write to output file ---
        // Only write the actions (no header, no blank line).
        out << turnStr << "\n";

        ++turn;
    }

    // --- Final result: console ---
    std::cout << "=== Final Board ===\n";
    std::cout << game_state_.getResultString() << "\n";

    // --- Final result: output file ---
    out << game_state_.getResultString() << "\n";

    out.close();
}
