#pragma once

#include <string>
#include <memory>
#include "GameState.h"                    // <– pull in the new GameState
#include "common/PlayerFactory.h"
#include "common/TankAlgorithmFactory.h"

namespace arena {

/*
  GameManager is now just a thin wrapper around GameState:
    1) It reads/parses the input file (map, MaxSteps, NumShells, Rows, Cols).
    2) Constructs a Board, then calls game_state_->initialize(...).
    3) Advances turn‐by‐turn via game_state_->step(...) and writes each output line.
*/
class GameManager {
public:
    GameManager(std::unique_ptr<common::PlayerFactory> pFac,
                std::unique_ptr<common::TankAlgorithmFactory> tFac);
    ~GameManager();

    void readBoard(const std::string& filename);
    void run();

private:
    // === new: just hold one GameState instance ===
    std::unique_ptr<GameState> game_state_;

    // To assemble GameState, we still need factory pointers:
    std::unique_ptr<common::PlayerFactory> player_factory_;
    std::unique_ptr<common::TankAlgorithmFactory> tank_factory_;

    // We need to remember the input‐filename so run() can open output_<inputfile>.txt
    std::string input_filename_;

    // === helpers for parsing lines like "MaxSteps = 5000" ===
    void parseKeyValue(const std::string& line, const std::string& key, std::size_t& out);
};

} // namespace arena
