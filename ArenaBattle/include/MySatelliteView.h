#pragma once

#include "SatelliteView.h"
#include <vector>
#include <cstddef>

/*
  A concrete SatelliteView implementation that holds a full 2D grid of chars.
  GameManager builds one of these each time a tank requests GetBattleInfo,
  copies the current board into “grid_,” then overwrites the querying tank’s
  cell with '%'.
*/
class MySatelliteView : public common::SatelliteView {
public:
    MySatelliteView(const std::vector<std::vector<char>>& board,
                    std::size_t rows,
                    std::size_t cols,
                    std::size_t query_x,
                    std::size_t query_y)
        : rows_(rows), cols_(cols), grid_(board)
    {
        if (query_x < rows_ && query_y < cols_) {
            grid_[query_x][query_y] = '%';
        }
    }

    virtual char getObjectAt(std::size_t x, std::size_t y) const override {
        if (x >= rows_ || y >= cols_) return '&';
        return grid_[x][y];
    }

private:
    std::size_t rows_, cols_;
    std::vector<std::vector<char>> grid_;
};
