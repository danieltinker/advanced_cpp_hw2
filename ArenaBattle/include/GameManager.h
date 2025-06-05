#pragma once

#include <memory>
#include <string>
#include "GameState.h"

namespace common {
    class PlayerFactory;
    class TankAlgorithmFactory;
}

namespace arena {

class GameManager {
public:
    // Construct with two factories; GameState is default‐constructed internally.
    GameManager(std::unique_ptr<common::PlayerFactory> pFac,
                std::unique_ptr<common::TankAlgorithmFactory> tFac);
    ~GameManager();

    // Load and parse <map_file>, build a Board, then initialize GameState.
    void readBoard(const std::string& map_file);

    // After readBoard(...), call run() to perform the game loop (no arguments).
    void run();

private:
    GameState game_state_;
    std::string loaded_map_file_;  // remembered so run() can name its output file

    // We will need to remember MaxSteps and NumShells during readBoard, so that
    // GameState.initialize(...) can be called. In practice, they are passed directly
    // inside readBoard, and not stored here as members.
};

} // namespace arena
