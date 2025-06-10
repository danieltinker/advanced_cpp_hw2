// include/MySatelliteView.h
#pragma once

#include "common/SatelliteView.h"
#include <vector>

namespace arena {

/// Concrete SatelliteView holding a full grid snapshot plus '%' at the querying tank.
class MySatelliteView : public common::SatelliteView {
public:
    /// @param board   current board snapshot (grid[y][x])
    /// @param rows    number of rows in the grid
    /// @param cols    number of columns
    /// @param queryX  x-coordinate of querying tank
    /// @param queryY  y-coordinate of querying tank
    MySatelliteView(const std::vector<std::vector<char>>& board,
                    std::size_t rows,
                    std::size_t cols,
                    std::size_t queryX,
                    std::size_t queryY)
      : rows_(rows), cols_(cols), grid_(board)
    {
        if (queryX < cols_ && queryY < rows_) {
            grid_[queryY][queryX] = '%';
        }
    }

    char getObjectAt(std::size_t x, std::size_t y) const override {
        if (x >= cols_ || y >= rows_) {
            return '&';
        }
        return grid_[y][x];
    }

private:
    std::size_t rows_, cols_;
    std::vector<std::vector<char>> grid_;
};

} // namespace arena
