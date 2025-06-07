#pragma once

#include "common/BattleInfo.h"
#include "common/ActionRequest.h"
#include "common/TankAlgorithm.h"
#include "common/SatelliteView.h"
// namespace arena { class TankAlgorithm; }

namespace common {

/**
 * Every “Player” must implement this interface.  When GameState sees
 * a tank asking for GetBattleInfo, it will call updateTankWithBattleInfo()
 * and pass in a reference to the tank’s TankAlgorithm plus a BattleInfo.
 */
class Player {
public:
    virtual ~Player() = default;
    virtual void updateTankWithBattleInfo(
        common::TankAlgorithm &tank,
        SatelliteView        &satellite_view
    ) = 0;
};

} // namespace common
