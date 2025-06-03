#pragma once

#include "BattleInfo.h"
#include <vector>
#include <cstddef>

/*
  A concrete BattleInfo containing:
    - rows, cols
    - a full 2D grid (vector<vector<char>>)
    - (optionally) my own x,y, direction, shells_left if you want to pass
      that to the tank. But by assignment, tanks learn position by scanning '%'.
*/
struct MyBattleInfo : public common::BattleInfo {
    std::size_t rows, cols;
    std::vector<std::vector<char>> grid;
};
