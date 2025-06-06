// In AggressiveTank.cpp:
#include "AggressiveTank.h"
#include <cassert>

AggressiveTank::AggressiveTank(int pIdx, int tIdx)
    : TankAlgorithm(pIdx, tIdx)
{}

AggressiveTank::~AggressiveTank() = default;

void AggressiveTank::updateBattleInfo(const common::BattleInfo& baseInfo) {
    // Down‐cast to our concrete type (we know GameState only passed MyBattleInfo)
    const MyBattleInfo& info = static_cast<const MyBattleInfo&>(baseInfo);

    // Copy it into our member for later getAction() logic
    lastInfo_ = info;

    // Initialize or refresh our own shell count
    shellsRemaining_ = info.shellsRemaining;
}

common::ActionRequest AggressiveTank::getAction() {
    // Example: if an enemy is directly in front (scan lastInfo_.grid),
    // and we have shells, then Shoot and decrement:
    // (pseudocode—replace with your real search)
    bool enemyAhead = false;
    {
        // Suppose “selfDir” 2 means “→,” so we check (selfY, selfX+1..)
        int dx = 0, dy = 0;
        switch ( lastInfo_.selfDir & 7 ) {
            case 0:  dy = -1; break;    // ↑
            case 1:  dx = +1; dy = -1; break; // ↗
            case 2:  dx = +1; break;    // →
            case 3:  dx = +1; dy = +1; break; // ↘
            case 4:  dy = +1; break;    // ↓
            case 5:  dx = -1; dy = +1; break; // ↙
            case 6:  dx = -1; break;    // ←
            case 7:  dx = -1; dy = -1; break; // ↖
        }
        int nx = static_cast<int>(lastInfo_.selfX) + dx;
        int ny = static_cast<int>(lastInfo_.selfY) + dy;
        if (nx >= 0 && nx < static_cast<int>(lastInfo_.cols) &&
            ny >= 0 && ny < static_cast<int>(lastInfo_.rows))
        {
            char c = lastInfo_.grid[ny][nx];
            // If that char is '1' or '2' and it isn’t “me” (’%’), it’s an enemy
            if ((c == '1' || c == '2') && c != '%') {
                enemyAhead = true;
            }
        }
    }

    if (enemyAhead && shellsRemaining_ > 0) {
        // We have ammo, shoot and decrement
        --shellsRemaining_;
        return common::ActionRequest::Shoot;
    }

    // Otherwise, for example, rotate toward some direction or move:
    return common::ActionRequest::MoveForward;
}
