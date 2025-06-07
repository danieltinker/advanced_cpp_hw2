// src/AggressiveTank.cpp
#include "AggressiveTank.h"
#include "common/ActionRequest.h"

using namespace arena;
using namespace common;

static constexpr int D8[8][2] = {
    { 0,-1}, {+1,-1}, {+1,0}, {+1,+1},
    { 0,+1}, {-1,+1}, {-1,0}, {-1,-1}
};

AggressiveTank::AggressiveTank(int playerIndex, int /*tankIndex*/)
  : lastInfo_{1,1},
    direction_(playerIndex == 1 ? 6 : 2)
{ }

void AggressiveTank::updateBattleInfo(BattleInfo &baseInfo) {
    auto &info = static_cast<MyBattleInfo&>(baseInfo);

    // first time only: grab our starting ammo
    if (shellsLeft_ == SIZE_MAX) {
        shellsLeft_ = info.shellsRemaining;
    }

    // copy in the new grid & self‐coordinates
    lastInfo_ = info;
}

ActionRequest AggressiveTank::getAction() {
    // on turn 0 we haven’t learned ammo yet
    if (shellsLeft_ == SIZE_MAX) {
        return ActionRequest::GetBattleInfo;
    }

    // look one cell ahead in direction_
    int nx = int(lastInfo_.selfX) + D8[direction_][0];
    int ny = int(lastInfo_.selfY) + D8[direction_][1];
    if (nx >= 0 && nx < int(lastInfo_.cols)
     && ny >= 0 && ny < int(lastInfo_.rows))
    {
        char c = lastInfo_.grid[ny][nx];
        if ((c=='1' || c=='2') && c!='%') {
            if (shellsLeft_ > 0) {
                --shellsLeft_;
                return ActionRequest::Shoot;
            }
        }
    }

    // fallback: move forward
    return ActionRequest::MoveForward;
}
