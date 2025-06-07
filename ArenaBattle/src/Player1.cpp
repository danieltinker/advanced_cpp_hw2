#include "Player1.h"

using namespace arena;
using namespace common;

Player1::Player1(int /*player_index*/,
                 std::size_t rows,
                 std::size_t cols,
                 std::size_t /*max_steps*/,
                 std::size_t num_shells)
  : rows_(rows)
  , cols_(cols)
  , initialShells_(num_shells)
  , firstInfo_(true)
{}

void Player1::updateTankWithBattleInfo(
    TankAlgorithm  &tank,
    SatelliteView  &sv
) {
    // Build our info wrapper
    MyBattleInfo info(rows_, cols_);

    // Fill grid + self marker
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

    // Relay initial shells exactly once:
    if (firstInfo_) {
        info.shellsRemaining = initialShells_;
        firstInfo_ = false;
    }
    // otherwise leave info.shellsRemaining == 0 (unused)

    // Forward to tank algo
    tank.updateBattleInfo(info);
}
