#include "GameManager.h"
#include "MyPlayerFactory.h"
#include "MyTankAlgorithmFactory.h"
#include "utils.h"

#include <iostream>
#include <fstream>
#include <memory>
#include <string>

using namespace arena;
using namespace common;

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: tanks_game <input_file>\n";
        return 1;
    }
    std::string map_file = argv[1];

    // First pass: read Rows and Cols so we can construct MyPlayerFactory(rows, cols)
    {
        std::ifstream in(map_file);
        if (!in) {
            std::cerr << "Cannot open map file: " << map_file << "\n";
            return 1;
        }
        std::string line;

        // 1) Skip map name
        std::getline(in, line);

        // 2) MaxSteps line (ignore)
        std::getline(in, line);

        // 3) NumShells line (ignore for now)
        std::getline(in, line);

        // 4) Rows = <NUM>
        std::getline(in, line);
        std::size_t rows = 0;
        parseKeyValue(line, "Rows", rows);

        // 5) Cols = <NUM>
        std::getline(in, line);
        std::size_t cols = 0;
        parseKeyValue(line, "Cols", cols);

        in.close();

        // Now we have rows, cols
        // Next, reopen to read NumShells
        in.open(map_file);
        if (!in) {
            std::cerr << "Cannot reopen map file: " << map_file << "\n";
            return 1;
        }
        // skip map name, skip MaxSteps
        std::getline(in, line);
        std::getline(in, line);

        // 3) NumShells = <NUM>
        std::getline(in, line);
        std::size_t num_shells = 0;
        parseKeyValue(line, "NumShells", num_shells);

        in.close();

        // Construct both factories
        auto playerFac = std::make_unique<MyPlayerFactory>(rows, cols);
        auto tankFac   = std::make_unique<MyTankAlgorithmFactory>(num_shells);

        // Construct GameManager and run
        GameManager gm(std::move(playerFac), std::move(tankFac));
        gm.readBoard(map_file);
        gm.run();
    }

    return 0;
}
