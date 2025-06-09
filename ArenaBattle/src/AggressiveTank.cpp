// src/AggressiveTank.cpp
#include "AggressiveTank.h"
#include "common/ActionRequest.h"

using namespace arena;
using namespace common;


AggressiveTank::AggressiveTank(int playerIndex, int /*tankIndex*/)
  : lastInfo_{1,1},
    direction_(playerIndex == 1 ? 6 : 2)
{}

void AggressiveTank::updateBattleInfo(BattleInfo &baseInfo) {
    auto &info = static_cast<MyBattleInfo&>(baseInfo);
    if (shellsLeft_ == SIZE_MAX) {
        shellsLeft_ = info.shellsRemaining;
    }
    lastInfo_ = info;
}

ActionRequest AggressiveTank::getAction() {
    // 1) First time ever: ask for shell count
    if (shellsLeft_ == SIZE_MAX) {
        return ActionRequest::Shoot;
    }

    // 2) Scan our entire row for any opponent ('1' or '2'), excluding ourselves ("%")
    int y = int(lastInfo_.selfY);
    if (shellsLeft_ > 0) {
        for (int x = 0; x < int(lastInfo_.cols); ++x) {
            if (x == int(lastInfo_.selfX)) continue;
            char c = lastInfo_.grid[y][x];
            if ((c == '1' || c == '2') && c != '%') {
                // enemy in the same row → shoot
                --shellsLeft_;
                return ActionRequest::Shoot;
            }
        }
    }

    // 3) No target in row (or no ammo) → move straight ahead
    return ActionRequest::Shoot;
}