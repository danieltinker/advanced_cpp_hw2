#include "GameManager.h"
#include "utils.h"

#include <fstream>
#include <sstream>
#include <iostream>

using namespace arena;
using namespace common;

// Two‐argument constructor remains the same
GameManager::GameManager(std::unique_ptr<PlayerFactory> pFac,
                         std::unique_ptr<TankAlgorithmFactory> tFac)
    : player_factory_(std::move(pFac)),
      tank_factory_(std::move(tFac))
{
    // Nothing else here; readBoard(...) will do the actual initialization.
}

GameManager::~GameManager() = default;

void GameManager::readBoard(const std::string& map_file) {
    // (1) Open map_file and parse header fields:
    std::ifstream in(map_file);
    if (!in) {
        std::cerr << "Cannot open map file: " << map_file << "\n";
        std::exit(1);
    }

    std::string line;
    // Skip map name
    std::getline(in, line);

    // MaxSteps = <NUM>
    std::getline(in, line);
    std::size_t max_steps = 0;
    parseKeyValue(line, "MaxSteps", max_steps);

    // NumShells = <NUM>
    std::getline(in, line);
    std::size_t num_shells = 0;
    parseKeyValue(line, "NumShells", num_shells);

    // Rows = <NUM>
    std::getline(in, line);
    std::size_t rows = 0;
    parseKeyValue(line, "Rows", rows);

    // Cols = <NUM>
    std::getline(in, line);
    std::size_t cols = 0;
    parseKeyValue(line, "Cols", cols);

    // Read the next 'rows' lines into a Board
    Board board(static_cast<int>(rows), static_cast<int>(cols));
    std::size_t r = 0;
    while (r < rows && std::getline(in, line)) {
        for (std::size_t c = 0; c < cols && c < line.size(); ++c) {
            char ch = line[c];
            if (ch == '#' || ch == '@' || ch == '1' || ch == '2') {
                board.setCell(static_cast<int>(c),
                              static_cast<int>(r),
                              CellContent(ch));
            } else {
                board.setCell(static_cast<int>(c),
                              static_cast<int>(r),
                              CellContent::EMPTY);
            }
        }
        ++r;
    }
    in.close();

    // (2) Initialize GameState now that we have Board, max_steps, num_shells
    game_state_.initialize(board, max_steps, num_shells);

    // (3) Store basename and output filename for later run()
    //     e.g. if map_file="/path/to/input_b.txt", stored_basename_ = "input_b.txt"
    std::string basename = map_file;
    auto slash_pos = basename.find_last_of("/\\");
    if (slash_pos != std::string::npos) {
        basename = basename.substr(slash_pos + 1);
    }
    stored_basename_ = basename;
    stored_output_filename_ = "output_" + basename;
}

void GameManager::run() {
    // We assume readBoard(...) was already called, so game_state_ is initialized
    // and stored_output_filename_ is set.

    // Open the output file we stored earlier:
    std::ofstream out(stored_output_filename_);
    if (!out) {
        std::cerr << "Cannot open output file: " << stored_output_filename_ << "\n";
        std::exit(1);
    }

    // Exactly one comma‐separated line per turn
    while (!game_state_.isGameOver()) {
        std::string turn_line = game_state_.advanceOneTurn();
        out << turn_line << "\n";
    }

    // Final result string
    out << game_state_.getResultString() << "\n";
    out.close();
}
