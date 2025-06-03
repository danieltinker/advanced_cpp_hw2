#pragma once

#include <vector>
#include <cstddef>

/*
 A simple Board representation: holds a 2D grid of chars (rows × cols).
 '#' = wall, '@' = mine, '1'/'2' = tank, ' ' = empty.
*/
class Board {
public:
    Board() = default;
    Board(std::size_t rows, std::size_t cols)
        : rows_(rows), cols_(cols), grid_(rows, std::vector<char>(cols, ' '))
    {}

    std::size_t getRows() const { return rows_; }
    std::size_t getCols() const { return cols_; }

    // Accessors:
    void setCell(std::size_t r, std::size_t c, char val) {
        if (r < rows_ && c < cols_) grid_[r][c] = val;
    }
    char getCell(std::size_t r, std::size_t c) const {
        if (r >= rows_ || c >= cols_) return '&';
        return grid_[r][c];
    }

    const std::vector<std::vector<char>>& getGrid() const {
        return grid_;
    }
    std::vector<std::vector<char>>& getGrid() {
        return grid_;
    }

private:
    std::size_t rows_{0}, cols_{0};
    std::vector<std::vector<char>> grid_;
};
