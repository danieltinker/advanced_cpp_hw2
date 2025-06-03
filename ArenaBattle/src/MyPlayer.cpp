#include "MyPlayer.h"

namespace arena {

MyPlayer::MyPlayer(int player_index,
                   size_t startX, size_t startY,
                   size_t max_steps, size_t num_shells)
    : common::Player(player_index, startX, startY, max_steps, num_shells),
      playerIndex_(player_index),
      maxSteps_(max_steps),
      numShells_(num_shells)
{
    // No implementation yet
}

void MyPlayer::updateTankWithBattleInfo(
    common::TankAlgorithm& tank,
    common::SatelliteView& satellite_view
) {
    // TODO: build a MyBattleInfo from SatelliteView and call tank.updateBattleInfo(...)
}

} // namespace arena
