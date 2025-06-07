// include/Player2.h
#pragma once

#include "common/Player.h"
#include "common/SatelliteView.h"
#include "common/TankAlgorithm.h"
#include "MyBattleInfo.h"

namespace arena {

/**
 * Player2: wraps the provided SatelliteView into a MyBattleInfo
 * and forwards it to its TankAlgorithm.
 */
class Player2 : public common::Player {
public:
    Player2(int player_index,
            std::size_t rows,
            std::size_t cols,
            std::size_t max_steps,
            std::size_t num_shells);

    ~Player2() override = default;

    void updateTankWithBattleInfo(
        common::TankAlgorithm   &tank,
        common::SatelliteView   &sv
    ) override;

private:
    std::size_t rows_, cols_;
};

} // namespace arena
