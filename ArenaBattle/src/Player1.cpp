// src/Player1.cpp
#include "Player1.h"

using namespace arena;
using namespace common;

Player1::Player1(int player_index,
                 std::size_t rows,
                 std::size_t cols,
                 std::size_t max_steps,
                 std::size_t num_shells)
  : common::Player()   // base has only a default ctor
  , rows_(rows)
  , cols_(cols)
{}

void Player1::updateTankWithBattleInfo(common::TankAlgorithm &tank,
                                       common::SatelliteView &sv)
{
    MyBattleInfo info(rows_, cols_);

    // Fill the grid and record self coordinates
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

    tank.updateBattleInfo(info);
}
