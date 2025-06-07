// src/EvasiveTank.cpp
#include "EvasiveTank.h"
#include "common/ActionRequest.h"

using namespace arena;
using namespace common;

static constexpr int D8[8][2] = {
    { 0,-1}, {+1,-1}, {+1,0}, {+1,+1},
    { 0,+1}, {-1,+1}, {-1,0}, {-1,-1}
};

EvasiveTank::EvasiveTank(int playerIndex, int /*tankIndex*/)
  : lastInfo_{1,1},
    direction_(playerIndex == 1 ? 6 : 2)
{ }

void EvasiveTank::updateBattleInfo(BattleInfo &baseInfo) {
    auto &info = static_cast<MyBattleInfo&>(baseInfo);

    // first time only: learn ammo
    if (shellsLeft_ == SIZE_MAX) {
        shellsLeft_ = info.shellsRemaining;
    }

    // update grid + self coordinates
    lastInfo_ = info;
}

ActionRequest EvasiveTank::getAction() {
    // turn 0: get shell count
    if (shellsLeft_ == SIZE_MAX) {
        return ActionRequest::GetBattleInfo;
    }

    // (1) if any adjacent cell has a shell overlay '*', back up
    for (int dir = 0; dir < 8; ++dir) {
        int nx = int(lastInfo_.selfX) + D8[dir][0];
        int ny = int(lastInfo_.selfY) + D8[dir][1];
        if (nx>=0 && nx<int(lastInfo_.cols)
         && ny>=0 && ny<int(lastInfo_.rows)
         && lastInfo_.grid[ny][nx] == '*')
        {
            return ActionRequest::MoveBackward;
        }
    }

    // (2) if the cell in our own direction_ is wall or mine → rotate
    int nx = int(lastInfo_.selfX) + D8[direction_][0];
    int ny = int(lastInfo_.selfY) + D8[direction_][1];
    if (nx>=0 && nx<int(lastInfo_.cols)
     && ny>=0 && ny<int(lastInfo_.rows))
    {
        char c = lastInfo_.grid[ny][nx];
        if (c == '#' || c == '@') {
            return ActionRequest::RotateLeft90;
        }
    }

    // (3) otherwise drive forward
    return ActionRequest::MoveForward;
}
