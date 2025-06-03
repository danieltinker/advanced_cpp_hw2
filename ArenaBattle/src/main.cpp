#include "GameManager.h"

#include <iostream>
#include <memory>
#include <string>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: tanks_game <input_file>\n";
        return 1;
    }
    std::string map_file = argv[1];

    // We must read “Rows” and “Cols” from the file before constructing MyPlayerFactory.
    std::ifstream in(map_file);
    if (!in) {
        std::cerr << "Cannot open map file: " << map_file << "\n";
        return 1;
    }
    std::string line;
    // Skip map name:
    std::getline(in, line);
    // MaxSteps:
    std::getline(in, line);
    // NumShells:
    std::getline(in, line);
    // Rows:
    std::getline(in, line);
    std::size_t rows = 0;
    parseKeyValue(line, "Rows", rows);
    // Cols:
    std::getline(in, line);
    std::size_t cols = 0;
    parseKeyValue(line, "Cols", cols);
    in.close();

    // Re‐open to get NumShells (for TankAlgorithmFactory):
    in.open(map_file);
    std::getline(in, line); // map name
    std::getline(in, line); // MaxSteps
    std::getline(in, line); // NumShells
    std::size_t num_shells = 0;
    parseKeyValue(line, "NumShells", num_shells);
    in.close();

    // Construct the two factories:
    auto playerFac = std::make_unique<MyPlayerFactory>(rows, cols);
    auto tankFac   = std::make_unique<MyTankAlgorithmFactory>(num_shells);

    // Create GameManager and run:
    GameManager gm(std::move(playerFac), std::move(tankFac));
    gm.readBoard(map_file);
    gm.run();

    return 0;
}
