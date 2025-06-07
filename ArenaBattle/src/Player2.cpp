#include "Player2.h"

using namespace arena;
using namespace common;

Player2::Player2(int /*player_index*/,
                 std::size_t rows,
                 std::size_t cols,
                 std::size_t /*max_steps*/,
                 std::size_t num_shells)
  : rows_(rows)
  , cols_(cols)
  , initialShells_(num_shells)
  , firstInfo_(true)
{}

void Player2::updateTankWithBattleInfo(
    TankAlgorithm  &tank,
    SatelliteView  &sv
) {
    MyBattleInfo info(rows_, cols_);

    for (std::size_t y = 0; y < rows_; ++y) {
        for (std::size_t x = 0; x < cols_; ++x) {
            char c = sv.getObjectAt(x, y);
            info.grid[y][x] = c;
            if (c == '%') {
                info.selfX = x;
                info.selfY = y;
            }
        }
    }

    if (firstInfo_) {
        info.shellsRemaining = initialShells_;
        firstInfo_ = false;
    }

    tank.updateBattleInfo(info);
}
