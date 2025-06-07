// src/Board.cpp
#include "Board.h"

Board::Board(std::size_t rows, std::size_t cols)
  : rows_(rows), cols_(cols),
    grid_(rows, std::vector<Cell>(cols))
{}

void Board::setCell(int x, int y, CellContent c) {
    Cell& cell = grid_[y][x];
    cell.content         = c;
    cell.wallHits        = (c == CellContent::WALL ? 0 : 0);
    cell.hasShellOverlay = false;
}

void Board::wrapCoords(int& x, int& y) const {
    int w = int(cols_), h = int(rows_);
    x = (x % w + w) % w;
    y = (y % h + h) % h;
}

void Board::clearShellMarks() {
    for (auto& row : grid_)
        for (auto& cell : row)
            cell.hasShellOverlay = false;
}
