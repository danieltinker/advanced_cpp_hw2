#include "GameManager.h"

#include <iostream>
#include <memory>
#include <string>
#include <fstream>

#include "utils.h"
#include "MyPlayerFactory.h"
#include "MyTankAlgorithmFactory.h"

using namespace arena;
using namespace common;

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: tanks_game <input_file>\n";
        return 1;
    }
    std::string map_file = argv[1];

    // Step A: Read header so we can build factories
    std::ifstream in(map_file);
    if (!in) {
        std::cerr << "Cannot open map file: " << map_file << "\n";
        return 1;
    }

    std::string line;
    // (1) Skip map name
    std::getline(in, line);

    // (2) MaxSteps
    std::getline(in, line);
    std::size_t max_steps = 0;
    parseKeyValue(line, "MaxSteps", max_steps);

    // (3) NumShells
    std::getline(in, line);
    std::size_t num_shells = 0;
    parseKeyValue(line, "NumShells", num_shells);

    // (4) Rows
    std::getline(in, line);
    std::size_t rows = 0;
    parseKeyValue(line, "Rows", rows);

    // (5) Cols
    std::getline(in, line);
    std::size_t cols = 0;
    parseKeyValue(line, "Cols", cols);

    in.close();

    // Step B: Construct the two factories
    auto playerFac = std::make_unique<MyPlayerFactory>(rows, cols);
    auto tankFac   = std::make_unique<MyTankAlgorithmFactory>(num_shells);

    // Step C: Construct GameManager and initialize via readBoard()
    GameManager gm(std::move(playerFac), std::move(tankFac));
    gm.readBoard(map_file);

    // Step D: Run simulation (zero‐arg run())
    gm.run();

    return 0;
}
