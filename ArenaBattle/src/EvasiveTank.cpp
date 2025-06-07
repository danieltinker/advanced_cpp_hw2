// src/EvasiveTank.cpp
#include "EvasiveTank.h"
#include <algorithm>
#include <cmath>

using namespace arena;

EvasiveTank::EvasiveTank(int pIdx, int tIdx)
    : TankAlgorithm(pIdx, tIdx)
{}

EvasiveTank::~EvasiveTank(){};

void EvasiveTank::updateBattleInfo(common::BattleInfo& baseInfo) {
    const MyBattleInfo& info = static_cast<const MyBattleInfo&>(baseInfo);
    lastInfo_       = info;
    shellsRemaining_= info.shellsRemaining;
}

common::ActionRequest EvasiveTank::getAction() {
    // (1) If there’s any '*' (shell overlay) in one of the neighboring cells,
    //     try to move/dodge sideways. We’ll check all 8 neighbors for '*'.
    for (int dir = 0; dir < 8; ++dir) {
        int dx = 0, dy = 0;
        switch (dir) {
            case 0:  dy = -1; break;
            case 1:  dx = +1; dy = -1; break;
            case 2:  dx = +1; break;
            case 3:  dx = +1; dy = +1; break;
            case 4:  dy = +1; break;
            case 5:  dx = -1; dy = +1; break;
            case 6:  dx = -1; break;
            case 7:  dx = -1; dy = -1; break;
        }
        int nx = static_cast<int>(lastInfo_.selfX) + dx;
        int ny = static_cast<int>(lastInfo_.selfY) + dy;
        if (nx >= 0 && nx < static_cast<int>(lastInfo_.cols) &&
            ny >= 0 && ny < static_cast<int>(lastInfo_.rows))
        {
            if (lastInfo_.grid[ny][nx] == '*') {
                // Shell is about to hit us if we stand still—so back up or rotate
                return common::ActionRequest::MoveBackward;
            }
        }
    }

    // (2) If no immediate shell threat, but we see a wall in front, rotate to avoid:
    {
        int dx = 0, dy = 0;
        switch (lastInfo_.selfDir & 7) {
            case 0:  dy = -1; break;
            case 1:  dx = +1; dy = -1; break;
            case 2:  dx = +1; break;
            case 3:  dx = +1; dy = +1; break;
            case 4:  dy = +1; break;
            case 5:  dx = -1; dy = +1; break;
            case 6:  dx = -1; break;
            case 7:  dx = -1; dy = -1; break;
        }
        int nx = static_cast<int>(lastInfo_.selfX) + dx;
        int ny = static_cast<int>(lastInfo_.selfY) + dy;
        if (nx >= 0 && nx < static_cast<int>(lastInfo_.cols) &&
            ny >= 0 && ny < static_cast<int>(lastInfo_.rows))
        {
            char c = lastInfo_.grid[ny][nx];
            if (c == '#' || c == '@') {
                // If there’s a wall or mine directly ahead, rotate 90° left:
                return common::ActionRequest::RotateLeft90;
            }
        }
    }

    // (3) Otherwise, just move forward slowly to stay out of danger
    return common::ActionRequest::MoveForward;
}
