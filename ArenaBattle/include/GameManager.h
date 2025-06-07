#pragma once

#include <memory>
#include <string>
#include "GameState.h"

namespace common {
    class PlayerFactory;
    class TankAlgorithmFactory;
}

namespace arena {

/// Loads a map, initializes GameState, and runs the main loop.
class GameManager {
public:
    /// Takes ownership of two factories.
    GameManager(std::unique_ptr<common::PlayerFactory>        pFac,
                std::unique_ptr<common::TankAlgorithmFactory> tFac);
    ~GameManager() = default;

    /// Parses the map file and initializes GameState.
    void readBoard(const std::string& map_file);

    /// Executes the game loop until completion.
    void run();

private:
    GameState    game_state_;
    std::string  loaded_map_file_;
};

} // namespace arena
