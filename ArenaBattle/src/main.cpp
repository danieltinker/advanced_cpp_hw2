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

    std::ifstream in(map_file);
    if (!in) {
        std::cerr << "Cannot open map file: " << map_file << "\n";
        return 1;
    }

    std::string line;
    std::size_t max_steps = 0, num_shells = 0, rows = 0, cols = 0;

    // 1) Line 1: title (ignored, but must exist)
    if (!std::getline(in, line)) {
        std::cerr << "Invalid map file: missing title line\n";
        return 1;
    }

    // 2) Line 2: MaxSteps = <NUM>
    if (!std::getline(in, line) || !parseKeyValue(line, "MaxSteps", max_steps)) {
        std::cerr << "Invalid header (MaxSteps): \"" << line << "\"\n";
        return 1;
    }

    // 3) Line 3: NumShells = <NUM>
    if (!std::getline(in, line) || !parseKeyValue(line, "NumShells", num_shells)) {
        std::cerr << "Invalid header (NumShells): \"" << line << "\"\n";
        return 1;
    }

    // 4) Line 4: Rows = <NUM>
    if (!std::getline(in, line) || !parseKeyValue(line, "Rows", rows)) {
        std::cerr << "Invalid header (Rows): \"" << line << "\"\n";
        return 1;
    }

    // 5) Line 5: Cols = <NUM>
    if (!std::getline(in, line) || !parseKeyValue(line, "Cols", cols)) {
        std::cerr << "Invalid header (Cols): \"" << line << "\"\n";
        return 1;
    }

    in.close();

    // Build the two factories with the parsed parameters
    auto playerFac = std::make_unique<MyPlayerFactory>();
    auto tankFac   = std::make_unique<common::MyTankAlgorithmFactory>();

    // Construct, initialize, and run:
    GameManager gm(std::move(playerFac), std::move(tankFac));
    gm.readBoard(map_file);
    gm.run();

    return 0;
}
