#pragma once

#include <string>

#include "GameState.h"

namespace arena {

class GameManager {
public:
    // (1) Now that GameState owns both factories, GameManager's ctor just constructs game_state_:
    GameManager(std::unique_ptr<common::PlayerFactory> pFac,
                std::unique_ptr<common::TankAlgorithmFactory> tFac);

    ~GameManager();

    /// Reads the map file, initializes GameState, and stores output filename.
    void readBoard(const std::string& map_file);

    /// Runs the simulation (writes one line per turn into output_<basename>.txt).
    void run();

private:
    GameState game_state_;
    std::string stored_basename_;
    std::string stored_output_filename_;
};

} // namespace arena
