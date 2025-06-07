#include "Player2.h"

namespace arena {

Player2::Player2(int player_index,
                 size_t rows,
                 size_t cols,
                 size_t max_steps,
                 size_t num_shells)
  : common::Player(),  // default
    rows_(rows),
    cols_(cols)
{}

void Player2::updateTankWithBattleInfo(
    common::TankAlgorithm  &tank,
    common::SatelliteView  &sv
) {
    MyBattleInfo info(rows_, cols_);

    for (size_t y = 0; y < rows_; ++y) {
        for (size_t x = 0; x < cols_; ++x) {
            info.grid[y][x] = sv.getObjectAt(x, y);
        }
    }

    for (size_t y = 0; y < rows_; ++y) {
        for (size_t x = 0; x < cols_; ++x) {
            if (info.grid[y][x] == '%') {
                info.selfX = x;
                info.selfY = y;
                goto found2;
            }
        }
    }
found2:

    // (Optional) set info.selfDir, info.shellsRemaining, info.turnNumber

    tank.updateBattleInfo(info);
}

} // namespace arena
