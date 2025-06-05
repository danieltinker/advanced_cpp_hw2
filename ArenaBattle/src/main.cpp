#include "GameManager.h"

#include <iostream>
#include <memory>
#include <string>
#include <fstream>      // <-- add this, so std::ifstream is defined

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

    // Step A: read the key values first so we can construct factories
    std::ifstream in(map_file);            // now <fstream> is included
    if (!in) {
        std::cerr << "Cannot open map file: " << map_file << "\n";
        return 1;
    }
    std::string line;

    // Skip map name:
    std::getline(in, line);

    // MaxSteps:
    std::getline(in, line);
    std::size_t max_steps = 0;
    parseKeyValue(line, "MaxSteps", max_steps);

    // NumShells:
    std::getline(in, line);
    std::size_t num_shells = 0;
    parseKeyValue(line, "NumShells", num_shells);

    // Rows:
    std::getline(in, line);
    std::size_t rows = 0;
    parseKeyValue(line, "Rows", rows);

    // Cols:
    std::getline(in, line);
    std::size_t cols = 0;
    parseKeyValue(line, "Cols", cols);

    in.close();

    // Step B: construct factories
    auto playerFac = std::make_unique<MyPlayerFactory>(rows, cols);
    auto tankFac   = std::make_unique<MyTankAlgorithmFactory>(num_shells);

    // Step C: create GameManager, call readBoard, then run()
    GameManager gm(std::move(playerFac), std::move(tankFac));
    gm.readBoard(map_file);
    gm.run();

    return 0;
}
