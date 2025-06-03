#include "MyBattleInfo.h"

namespace arena {

MyBattleInfo::MyBattleInfo(size_t rows, size_t cols)
    : rows_(rows), cols_(cols),
      grid_(rows, std::vector<char>(cols, ' '))
{}

void MyBattleInfo::setObjectAt(size_t x, size_t y, char c) {
    if (x < rows_ && y < cols_) {
        grid_[x][y] = c;
    }
}

char MyBattleInfo::getObjectAt(size_t x, size_t y) const {
    if (x < rows_ && y < cols_) {
        return grid_[x][y];
    }
    return '&'; // outside battlefield
}

} // namespace arena
