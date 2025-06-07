// include/MyBattleInfo.h
#pragma once
#include "common/BattleInfo.h"
#include <vector>
#include <cstddef>

namespace arena {

/*
  Concrete BattleInfo: carries a 2D char‐grid plus the querying tank’s (x,y)
  and the engine’s shellsRemaining (once per turn).
*/
struct MyBattleInfo : public common::BattleInfo {
    std::size_t rows, cols;
    std::vector<std::vector<char>> grid;
    std::size_t selfX, selfY;        // tank’s (x,y)
    std::size_t shellsRemaining;     // engine’s current count

    MyBattleInfo(std::size_t r, std::size_t c)
      : rows(r), cols(c),
        grid(r, std::vector<char>(c,' ')),
        selfX(0), selfY(0),
        shellsRemaining(0)
    {}
};

} // namespace arena
