// src/EvasiveTank.cpp
#include "EvasiveTank.h"
#include "common/ActionRequest.h"

using namespace arena;
using namespace common;

EvasiveTank::EvasiveTank(int playerIndex, int /*tankIndex*/)
  : lastInfo_{1,1},
    shellsLeft_(-1),
    direction_(playerIndex == 1 ? 6 : 2)
{}

void EvasiveTank::updateBattleInfo(BattleInfo& baseInfo) {
    auto &info = static_cast<MyBattleInfo&>(baseInfo);
    // learn shells once
    if (shellsLeft_ < 0) shellsLeft_ = info.shellsRemaining;
    lastInfo_ = info;
}

ActionRequest EvasiveTank::getAction() {
    // always refresh view first
    static bool needView = true;
    if (needView) {
        needView = false;
        return ActionRequest::GetBattleInfo;
    }
    needView = true;

    int sx = int(lastInfo_.selfX), sy = int(lastInfo_.selfY);
    int R  = int(lastInfo_.rows), C  = int(lastInfo_.cols);

    // (1) dodge any adjacent shell overlay '*'
    for (int d = 0; d < 8; ++d) {
        int nx = sx + D8[d][0], ny = sy + D8[d][1];
        if (nx>=0 && nx<C && ny>=0 && ny<R && lastInfo_.grid[ny][nx]=='*') {
            // attempt backward, but avoid stepping on mines/walls/OOB
            int backDir = (direction_ + 4) & 7;
            int bx = sx + D8[backDir][0], by = sy + D8[backDir][1];
            if (bx>=0 && bx<C && by>=0 && by<R) {
                char c = lastInfo_.grid[by][bx];
                if (c!='#' && c!='@') {
                    return ActionRequest::MoveBackward;
                }
            }
            // can't retreat safely → pivot instead
            return ActionRequest::RotateLeft90;
        }
    }

    // (2) if forward cell is wall or mine → rotate
    {
        int fx = sx + D8[direction_][0];
        int fy = sy + D8[direction_][1];
        if (fx>=0 && fx<C && fy>=0 && fy<R) {
            char c = lastInfo_.grid[fy][fx];
            if (c=='#' || c=='@') {
                return ActionRequest::RotateLeft90;
            }
        }
    }

    // (3) safe to go forward
    return ActionRequest::MoveForward;
}
