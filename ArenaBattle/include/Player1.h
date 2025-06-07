#pragma once

#include "common/Player.h"
#include "common/SatelliteView.h"
#include "common/TankAlgorithm.h"
#include "MyBattleInfo.h"

namespace arena {

/**
 * Player1: constructs a MyBattleInfo from the SatelliteView
 * and forwards it to its tank algorithm.
 */
class Player1 : public common::Player {
public:
    // We still take the 5 args so PlayerFactory can invoke us:
    Player1(int player_index,
            size_t rows,
            size_t cols,
            size_t max_steps,
            size_t num_shells);

    ~Player1() override = default;

    // Exactly match common::Player:
    void updateTankWithBattleInfo(
        common::TankAlgorithm  &tank,
        common::SatelliteView  &satellite_view
    ) override;

private:
    size_t rows_, cols_;
    // (you can also store max_steps_ and num_shells_ here if you need them)
};

} // namespace arena
