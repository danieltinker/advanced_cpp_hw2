// include/Player1.h
#pragma once

#include "common/Player.h"
#include "common/SatelliteView.h"
#include "common/TankAlgorithm.h"
#include "MyBattleInfo.h"

namespace arena {

/**
 * Player1: wraps the provided SatelliteView into a MyBattleInfo
 * and forwards it to its TankAlgorithm.
 */
class Player1 : public common::Player {
public:
    Player1(int player_index,
            std::size_t rows,
            std::size_t cols,
            std::size_t max_steps,
            std::size_t num_shells);

    ~Player1() override = default;

    void updateTankWithBattleInfo(
        common::TankAlgorithm   &tank,
        common::SatelliteView   &sv
    ) override;

private:
    std::size_t rows_, cols_;
};

} // namespace arena
