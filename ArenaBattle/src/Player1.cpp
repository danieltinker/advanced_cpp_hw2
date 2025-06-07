#include "Player1.h"

namespace arena {

Player1::Player1(int player_index,
                 size_t rows,
                 size_t cols,
                 size_t max_steps,
                 size_t num_shells)
  : common::Player(),   // call default Player() ctor
    rows_(rows),
    cols_(cols)
{
    // Store or ignore max_steps/num_shells as you like:
    // this->max_steps_ = max_steps; etc.
}

void Player1::updateTankWithBattleInfo(
    common::TankAlgorithm  &tank,
    common::SatelliteView  &sv
) {
    // 1) Build a battle‐info wrapper
    MyBattleInfo info(rows_, cols_);

    // 2) Fill grid from satellite view
    for (size_t y = 0; y < rows_; ++y) {
        for (size_t x = 0; x < cols_; ++x) {
            info.grid[y][x] = sv.getObjectAt(x, y);
        }
    }

    // 3) Locate '%' for self position
    for (size_t y = 0; y < rows_; ++y) {
        for (size_t x = 0; x < cols_; ++x) {
            if (info.grid[y][x] == '%') {
                info.selfX = x;
                info.selfY = y;
                goto found1;
            }
        }
    }
found1:

    // 4) (Optionally) set direction, shellsRemaining, turnNumber

    // 5) Forward to tank algorithm
    tank.updateBattleInfo(info);
}

} // namespace arena
