#pragma once

#include "common/Player.h"
#include "common/SatelliteView.h"
#include "common/TankAlgorithm.h"
#include "MyBattleInfo.h"

namespace arena {

/**
 * Player2: same pattern, different heuristics.
 */
class Player2 : public common::Player {
public:
    Player2(int player_index,
            size_t rows,
            size_t cols,
            size_t max_steps,
            size_t num_shells);

    ~Player2() override = default;

    void updateTankWithBattleInfo(
        common::TankAlgorithm  &tank,
        common::SatelliteView  &satellite_view
    ) override;

private:
    size_t rows_, cols_;
};

} // namespace arena
