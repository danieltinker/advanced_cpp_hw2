#include "Board.h"

//------------------------------------------------------------------------------
// Constructors
//------------------------------------------------------------------------------
Board::Board()
  : rows_(0), cols_(0), width(0), height(0), grid()
{}

Board::Board(std::size_t rows, std::size_t cols)
  : rows_(rows), cols_(cols),
    width(static_cast<int>(cols)),
    height(static_cast<int>(rows)),
    grid(rows, std::vector<Cell>(cols))
{}

//------------------------------------------------------------------------------
// Accessors
//------------------------------------------------------------------------------
std::size_t Board::getRows() const { return rows_; }
std::size_t Board::getCols() const { return cols_; }
int Board::getWidth()  const { return width; }
int Board::getHeight() const { return height; }

const std::vector<std::vector<Cell>>& Board::getGrid() const {
    return grid;
}

Cell& Board::getCell(int x, int y) {
    return grid[y][x];
}

const Cell& Board::getCell(int x, int y) const {
    return grid[y][x];
}

//------------------------------------------------------------------------------
// setCell: change content of (x,y), resetting wallHits if needed.
//------------------------------------------------------------------------------
void Board::setCell(int x, int y, CellContent c) {
    Cell& cell = grid[y][x];
    cell.content = c;
    if (c == CellContent::WALL) {
        cell.wallHits = 0;
    } else {
        cell.wallHits = 0;
    }
    cell.hasShellOverlay = false;
}

//------------------------------------------------------------------------------
// wrapCoords: modulo‐wrap each coordinate
//------------------------------------------------------------------------------
void Board::wrapCoords(int& x, int& y) const {
    x = (x % width + width) % width;
    y = (y % height + height) % height;
}

//------------------------------------------------------------------------------
// clearShellMarks: no shell overlay on any cell
//------------------------------------------------------------------------------
void Board::clearShellMarks() {
    for (std::size_t r = 0; r < rows_; ++r) {
        for (std::size_t c = 0; c < cols_; ++c) {
            grid[r][c].hasShellOverlay = false;
        }
    }
}

//------------------------------------------------------------------------------
// clearTankMarks: no‐op (unused)
//------------------------------------------------------------------------------
void Board::clearTankMarks() {
    // Tanks are tracked via CellContent == TANK1/TANK2; no extra flags needed.
}
