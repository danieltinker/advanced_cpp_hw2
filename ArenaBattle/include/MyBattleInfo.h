#pragma once
#include "common/BattleInfo.h"
#include <vector>
#include <cstddef>

namespace arena {

struct MyBattleInfo : public common::BattleInfo {
    std::size_t rows, cols;
    std::vector<std::vector<char>> grid;
    std::size_t selfX, selfY;      // tank’s own coord
    std::size_t shellsRemaining;   // <-- engine’s ammo count
  
    MyBattleInfo(std::size_t r, std::size_t c)
      : rows(r)
      , cols(c)
      , grid(r, std::vector<char>(c,' '))
      , selfX(0)
      , selfY(0)
      , shellsRemaining(0)
    {}
};

} // namespace arena
