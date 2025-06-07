// src/AggressiveTank.cpp
#include "AggressiveTank.h"
#include "common/TankAlgorithm.h"
#include <algorithm> // for std::abs, etc.
//why dont you keep num shells and max steps? the tank algorithm should expect shells remaining
using namespace arena;

AggressiveTank::AggressiveTank(int pIdx, int tIdx)
    : TankAlgorithm(pIdx, tIdx)
{}

AggressiveTank::~AggressiveTank() { };

void AggressiveTank::updateBattleInfo(common::BattleInfo& baseInfo) {
    const MyBattleInfo& info = static_cast<const MyBattleInfo&>(baseInfo);
    lastInfo_       = info;
    shellsRemaining_= info.shellsRemaining;
}

common::ActionRequest AggressiveTank::getAction() {
    // (1) If an enemy is directly in front and we have ammo → Shoot
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
        if ((c == '1' || c == '2') && c != '%') {
            if (shellsRemaining_ > 0) {
                --shellsRemaining_;
                return common::ActionRequest::Shoot;
            }
        }
    }

    // (2) Otherwise, rotate randomly or move forward
    // You could implement a search for nearest enemy. For brevity:
    return common::ActionRequest::MoveForward;
}
