#pragma once

#include "TankAlgorithm.h"
#include "SatelliteView.h"

namespace common {

/*
  A Player’s only job is: whenever a tank calls “GetBattleInfo,”
  GameManager invokes:
      player->updateTankWithBattleInfo(tankAlg, satelliteView);
  and then the Player must build a derived BattleInfo, and call
      tankAlg->updateBattleInfo(myDerivedInfo);
  No other GM‐access is allowed here.
*/
class Player {
public:
    Player(int player_index, std::size_t max_steps, std::size_t num_shells)
        : player_index_(player_index),
          max_steps_(max_steps),
          num_shells_(num_shells)
    {}

    virtual ~Player() {}

    // Called by GameManager when a tank requests GetBattleInfo.
    virtual void updateTankWithBattleInfo(
        TankAlgorithm& tankAlg,
        SatelliteView& satellite_view) = 0;

protected:
    int player_index_;
    std::size_t max_steps_;
    std::size_t num_shells_;
};

} // namespace common
