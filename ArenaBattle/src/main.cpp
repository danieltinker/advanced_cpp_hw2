#include "GameManager.h"
#include "utils.h"
#include "MyPlayerFactory.h"
#include "MyTankAlgorithmFactory.h"

#include <iostream>
#include <fstream>
#include <memory>
#include <string>

using namespace arena;

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: tanks_game <input_file>\n";
        return 1;
    }
    const std::string map_file = argv[1];

    // Open the file to read just the header values:
    std::ifstream in(map_file);
    if (!in) {
        std::cerr << "Cannot open map file: " << map_file << "\n";
        return 1;
    }
    std::string line;

    // Skip the map’s title line:
    std::getline(in, line);

    std::size_t max_steps = 0, num_shells = 0, rows = 0, cols = 0;

    // Read and parse each header line in turn:
    std::getline(in, line);
    parseKeyValue(line, "MaxSteps",  max_steps);

    std::getline(in, line);
    parseKeyValue(line, "NumShells", num_shells);

    std::getline(in, line);
    parseKeyValue(line, "Rows",      rows);

    std::getline(in, line);
    parseKeyValue(line, "Cols",      cols);

    in.close();

    // Build the two factories:
    auto playerFac = std::make_unique<MyPlayerFactory>();
    auto tankFac   = std::make_unique<common::MyTankAlgorithmFactory>();

    // Construct, initialize, and run:
    GameManager gm(std::move(playerFac), std::move(tankFac));
    gm.readBoard(map_file);
    gm.run();

    return 0;
}
