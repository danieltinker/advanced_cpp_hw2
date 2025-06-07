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
    /// Construct with concrete factories; template allows passing by value
    template<typename PF, typename TF>
    GameManager(PF pFac, TF tFac)
        : game_state_(
              std::make_unique<PF>(std::move(pFac)),
              std::make_unique<TF>(std::move(tFac))
          )
    {}

    ~GameManager() = default;

    /// Load and parse <map_file>, build a Board, then initialize GameState.
    void readBoard(const std::string &map_file);

    /// After readBoard(...), call run() to perform the game loop (no arguments).
    void run();

private:
    GameState game_state_;
    std::string loaded_map_file_;  ///< remembered so run() can name its output file
};
}