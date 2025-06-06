// MyBattleInfo.h
#pragma once

#include "common/BattleInfo.h"
#include <vector>
#include <cstddef>

namespace arena {

/*
  A concrete BattleInfo containing:
    - rows, cols
    - a full 2D grid (vector<vector<char>>)
    - the querying tank’s own (x,y), direction, shellsRemaining, turnNumber
*/
struct MyBattleInfo : public common::BattleInfo {
    // Grid dimensions & the 2D char‐grid:
    std::size_t rows, cols;
    std::vector<std::vector<char>> grid;

    // The querying tank’s state:
    std::size_t selfX, selfY;   // tank’s (x,y)
    int        selfDir;         // 0..7
    int        shellsRemaining; // how many shells that tank still has

    // (Optional) current turn number:
    int turnNumber;

    // Default constructor is fine for vector‐of‐vector initialization
    MyBattleInfo() = default;

    // Construct with dimensions; grid gets initialized to spaces:
    MyBattleInfo(std::size_t r, std::size_t c)
      : rows(r), cols(c),
        grid(r, std::vector<char>(c, ' ')),
        selfX(0), selfY(0),
        selfDir(0),
        shellsRemaining(0),
        turnNumber(0)
    {}

    // Set a character at (x,y) in our char‐grid
    void setObjectAt(std::size_t x, std::size_t y, char ch) {
        if (x < cols && y < rows) {
            grid[y][x] = ch;
        }
    }

    // Read a character from (x,y)
    char getObjectAt(std::size_t x, std::size_t y) const {
        if (x < cols && y < rows) {
            return grid[y][x];
        }
        return ' ';
    }
};

} // namespace arena
