#pragma once

#include "common/SatelliteView.h"
#include <vector>
#include <cstddef>

/*
 A “helper” implementation of SatelliteView that GameManager can use
 internally if it prefers. In this layout, we are using MySatelliteView
 (see MySatelliteView.h) instead, so you can leave this file blank or
 remove it. For completeness, here it is:
*/
class SatelliteViewImpl : public common::SatelliteView {
public:
    SatelliteViewImpl(const std::vector<std::vector<char>>& board,
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
