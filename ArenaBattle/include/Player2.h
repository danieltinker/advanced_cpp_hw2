#pragma once

#include "common/Player.h"
#include "common/SatelliteView.h"
#include "MyBattleInfo.h"

namespace arena {

class Player2 : public common::Player {
public:
    Player2(int player_index,
            std::size_t rows,
            std::size_t cols,
            std::size_t max_steps,
            std::size_t num_shells);

    ~Player2() override = default;

    void updateTankWithBattleInfo(
        common::TankAlgorithm  &tank,
        common::SatelliteView  &sv
    ) override;

private:
    std::size_t rows_, cols_;
    std::size_t initialShells_;
    bool        firstInfo_ = true;
};

} // namespace arena
