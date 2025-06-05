#pragma once

#include <vector>
#include <cstddef>

// -----------------------------------------------------------------------------
// Define each grid‐cell’s content: empty, wall, mine, or a tank (player 1 or 2).
// Each wall takes two hits before disappearing. We also track if a shell is
// currently overlayed on top of that cell (for printing).
// -----------------------------------------------------------------------------
enum class CellContent {
    EMPTY,
    WALL,
    MINE,
    TANK1,
    TANK2
};

struct Cell {
    CellContent content;
    int wallHits;          // how many hits this wall has taken (0 or 1 before breaking)
    bool hasShellOverlay;  // true if a shell is currently over this cell

    Cell()
      : content(CellContent::EMPTY), wallHits(0), hasShellOverlay(false)
    {}
};

class Board {
public:
    Board();
    Board(std::size_t rows, std::size_t cols);

    std::size_t getRows() const;
    std::size_t getCols() const;
    int         getWidth()  const;  // same as (int)cols
    int         getHeight() const;  // same as (int)rows

    // Return const reference to the entire grid (for snapshot‐building).
    const std::vector<std::vector<Cell>>& getGrid() const;

    // Access a single cell (non‐const and const overloads).
    Cell&       getCell(int x, int y);
    const Cell& getCell(int x, int y) const;

    // Modify a cell’s content (resetting wallHits if needed).
    void setCell(int x, int y, CellContent c);

    // If x or y go out of [0..width) or [0..height), wrap them around.
    void wrapCoords(int& x, int& y) const;

    // Clear hasShellOverlay on every cell.
    void clearShellMarks();

    // Clear any “tank” marking from the grid (unused in current design).
    void clearTankMarks();

private:
    std::size_t rows_, cols_;
    int         width, height;
    std::vector<std::vector<Cell>> grid;
};
