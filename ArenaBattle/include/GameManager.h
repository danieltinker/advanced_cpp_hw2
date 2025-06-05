#pragma once

#include <memory>
#include <string>

#include "GameState.h"
#include "common/PlayerFactory.h"
#include "common/TankAlgorithmFactory.h"

namespace arena {

class GameManager {
public:
    // Two‐argument constructor as before.
    GameManager(std::unique_ptr<common::PlayerFactory> pFac,
                std::unique_ptr<common::TankAlgorithmFactory> tFac);

    ~GameManager();

    /// Reads the map file, initializes GameState, and stores the output filename.
    void readBoard(const std::string& map_file);

    /// Runs the simulation (printing to stdout if desired) and writes exactly one
    /// line per turn (plus final result) into "output_<basename>.txt".
    void run();

private:
    GameState game_state_;
    std::unique_ptr<common::PlayerFactory>       player_factory_;
    std::unique_ptr<common::TankAlgorithmFactory> tank_factory_;

    std::string stored_basename_;     // e.g. "input_b.txt"
    std::string stored_output_filename_; // e.g. "output_input_b.txt"
};

} // namespace arena
