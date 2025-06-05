#include "GameManager.h"
#include <fstream>
#include <iostream>
#include <sstream>

using namespace arena;
using namespace common;

///--------------------------------------------------------------------------------
/// Constructor / Destructor
///--------------------------------------------------------------------------------
GameManager::GameManager(std::unique_ptr<common::PlayerFactory> pFac,
                         std::unique_ptr<common::TankAlgorithmFactory> tFac)
    : player_factory_(std::move(pFac)),
      tank_factory_(std::move(tFac))
{
}

GameManager::~GameManager() = default;

///--------------------------------------------------------------------------------
/// parseKeyValue: parses a line like "MaxSteps = 5000" (or "Rows=4") into out.
/// If the key doesn't match, or conversion fails, out is unchanged.
///--------------------------------------------------------------------------------
void GameManager::parseKeyValue(const std::string& line,
                                const std::string& key,
                                std::size_t& out)
{
    // find “key” in the line:
    auto pos = line.find(key);
    if (pos == std::string::npos) return;
    // find the '=' after key:
    auto eqPos = line.find('=', pos + key.size());
    if (eqPos == std::string::npos) return;

    // everything after '=' is the number (possibly with whitespace)
    std::string valstr = line.substr(eqPos + 1);
    std::istringstream iss(valstr);
    std::size_t v;
    if (iss >> v) {
        out = v;
    }
}

///--------------------------------------------------------------------------------
/// readBoard:
///   1) Open the input text file (map_file).
///   2) Read lines: map‐name (ignore), MaxSteps, NumShells, Rows, Cols.
///   3) Build a Board(rows, cols) and fill its grid from the next “rows” lines.
///   4) Hand off everything to GameState::initialize(...).
///--------------------------------------------------------------------------------
void GameManager::readBoard(const std::string& filename)
{
    input_filename_ = filename;

    std::ifstream in(filename);
    if (!in) {
        std::cerr << "Cannot open map file: " << filename << "\n";
        std::exit(1);
    }

    std::string line;
    // 1) map name (ignore)
    std::getline(in, line);

    // 2) MaxSteps = <NUM>
    std::getline(in, line);
    std::size_t max_steps = 0;
    parseKeyValue(line, "MaxSteps", max_steps);

    // 3) NumShells = <NUM>
    std::getline(in, line);
    std::size_t num_shells = 0;
    parseKeyValue(line, "NumShells", num_shells);

    // 4) Rows = <NUM>
    std::getline(in, line);
    std::size_t rows = 0;
    parseKeyValue(line, "Rows", rows);

    // 5) Cols = <NUM>
    std::getline(in, line);
    std::size_t cols = 0;
    parseKeyValue(line, "Cols", cols);

    // 6) Build the Board(rows, cols) and read the next “rows” lines:
    Board board(rows, cols);
    std::size_t r = 0;
    while (r < rows && std::getline(in, line)) {
        for (std::size_t c = 0; c < cols && c < line.size(); ++c) {
            char ch = line[c];
            if (ch == '#' || ch == '@' || ch == '1' || ch == '2') {
                board.setCell(r, c, ch);
            } else {
                board.setCell(r, c, ' ');
            }
        }
        ++r;
    }
    in.close();

    // 7) Now hand everything off to GameState:
    //    Pass in both factories (owned by GameManager) plus the board, max_steps, num_shells.
    game_state_ = std::make_unique<GameState>(
        std::move(player_factory_),
        std::move(tank_factory_)
    );
    game_state_->initialize(board, max_steps, num_shells);
}

///--------------------------------------------------------------------------------
/// run():
///   1) Open “output_<input_filename>.txt”
///   2) While not game_state_->isGameOver():
///        a)   Ask each tank for its ActionRequest (GameState already holds all
///             algorithms and players internally; it provides a helper to do “GetBattleInfo”)
///        b)   Pass those two actions to game_state_->step(a1, a2)
///        c)   Write one line into the output file (all tanks’ actions, with “(ignored)”
///             or “(killed)” as needed).  We let GameState tell us exactly what to print.
/// FIXME:    For simplicity, here we ask GameState to return a single comma‐separated
///           string for the entire turn.  (You can adapt that to print exactly as
///           the assignment spec requires.)
///   3) After the loop, print the final “<winner>” or “Tie…” line.
///--------------------------------------------------------------------------------
void GameManager::run()
{
    // Output file: “output_<inputfile>.txt”
    std::string out_name = "output_" + input_filename_ + ".txt";
    std::ofstream out(out_name);
    if (!out) {
        std::cerr << "Cannot open " << out_name << " for writing\n";
        return;
    }

    std::size_t turnCount = 0;
    while (!game_state_->isGameOver()) {
        std::string turnLine = game_state_->advanceOneTurn();

        // Print the board to stdout after this turn:
        std::cout << "=== Turn " << (turnCount + 1) << " ===\n";
        game_state_->printBoard();

        // Also write the turnLine to the output file:
        out << turnLine << "\n";

        ++turnCount;
    }

    // Once done, print the final board one more time (optional):
    std::cout << "=== Final Board ===\n";
    game_state_->printBoard();

    // Then write the final result line:
    out << game_state_->getResultString() << "\n";
    out.close();
}

