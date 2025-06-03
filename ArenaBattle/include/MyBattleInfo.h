#pragma once

#include "common/BattleInfo.h"
#include <vector>
#include <cstddef>

namespace arena {

/*
  A concrete BattleInfo containing:
    - rows, cols
    - a full 2D grid (vector<vector<char>>)
    - helper to set an object at (x,y)
*/
struct MyBattleInfo : public common::BattleInfo {
    std::size_t rows, cols;
    std::vector<std::vector<char>> grid;

    // Default constructor
    MyBattleInfo() = default;

    // Two‐argument constructor
    MyBattleInfo(std::size_t r, std::size_t c);

    // Set a character at (x,y)
    void setObjectAt(std::size_t x, std::size_t y, char c);

    char getObjectAt(size_t x, size_t y) const;

    // (Optionally) you might also want a getter, but typically
    // MyBattleInfo is passed to TankAlgorithm, and that code uses
    // grid[x][y] directly. If not, you can add:
    // char getObjectAt(std::size_t x, std::size_t y) const { return grid[x][y]; }
};

} // namespace arena
