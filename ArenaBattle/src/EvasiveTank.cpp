// src/EvasiveTank.cpp
#include "EvasiveTank.h"
#include "common/ActionRequest.h"

using namespace arena;
using namespace common;

EvasiveTank::EvasiveTank(int playerIndex, int /*tankIndex*/)
  : lastInfo_{1,1},
    direction_(playerIndex == 1 ? 2 : 6),
    needView_(true),
    enemyChar_(playerIndex == 1 ? '2' : '1')
{}

void EvasiveTank::updateBattleInfo(BattleInfo& baseInfo) {
    lastInfo_ = static_cast<MyBattleInfo&>(baseInfo);
}

common::ActionRequest EvasiveTank::getAction() {
    const int R = int(lastInfo_.rows), C = int(lastInfo_.cols);
    const int sx = int(lastInfo_.selfX), sy = int(lastInfo_.selfY);

    // 1) Always fetch fresh view first
    if (needView_) {
        needView_ = false;
        return ActionRequest::GetBattleInfo;
    }
    needView_ = true;

    // 1a) Edge‐case: backward wrap lands on an enemy tank → refresh view
    {
        int backDir = (direction_ + 4) & 7;
        int bx = sx + DX[backDir], by = sy + DY[backDir];
        // wrap around toroidally
        bx = (bx % C + C) % C;
        by = (by % R + R) % R;
        char c = lastInfo_.grid[by][bx];
        if (c == enemyChar_) {
            // avoid stepping onto them blindly
            return ActionRequest::GetBattleInfo;
        }
    }

    // 2) Gather shell positions
    std::vector<std::pair<int,int>> shells;
    for (int y = 0; y < R; ++y) {
        for (int x = 0; x < C; ++x) {
            if (lastInfo_.grid[y][x] == '*')
                shells.emplace_back(x,y);
        }
    }

    // 3) Score each candidate action by min(turns to impact)
    struct Cand { ActionRequest act; int nx, ny, nd, score; };
    std::vector<Cand> cands;
    cands.reserve(7);

    // returns ceil(Chebyshev/2)
    auto danger = [&](int x, int y){
        int d = std::numeric_limits<int>::max();
        for (auto &sh : shells) {
            int dx = abs(sh.first - x);
            dx = std::min(dx, C - dx);
            int dy = abs(sh.second - y);
            dy = std::min(dy, R - dy);
            int cheb = std::max(dx,dy);
            d = std::min(d, (cheb + 1)/2);
        }
        return shells.empty() ? std::numeric_limits<int>::max() : d;
    };

    auto tryMove = [&](ActionRequest act, int nx, int ny, int nd){
        // legality: moves must avoid walls/mines; rotations & do-nothing always legal
        if (act == ActionRequest::MoveForward ||
            act == ActionRequest::MoveBackward)
        {
            if (nx < 0 || nx >= C || ny < 0 || ny >= R) return;
            char cc = lastInfo_.grid[ny][nx];
            if (cc == '#' || cc == '@') return;
        }
        cands.push_back({act, nx, ny, nd, danger(nx,ny)});
    };

    // MoveForward
    tryMove(ActionRequest::MoveForward,
            sx+DX[direction_], sy+DY[direction_], direction_);
    // MoveBackward
    {
        int d2=(direction_+4)&7;
        tryMove(ActionRequest::MoveBackward,
                sx+DX[d2],       sy+DY[d2],
                direction_);
    }
    // Rotations
    tryMove(ActionRequest::RotateLeft45,  sx, sy, (direction_+7)&7);
    tryMove(ActionRequest::RotateRight45, sx, sy, (direction_+1)&7);
    tryMove(ActionRequest::RotateLeft90,  sx, sy, (direction_+6)&7);
    tryMove(ActionRequest::RotateRight90, sx, sy, (direction_+2)&7);
    // DoNothing
    tryMove(ActionRequest::DoNothing, sx, sy, direction_);

    // 4) Pick max safety; tie‐break order favors backward, forward, Rot45L, Rot45R, Rot90L, Rot90R, DoNothing
    static constexpr ActionRequest order[] = {
      ActionRequest::MoveBackward,
      ActionRequest::MoveForward,
      ActionRequest::RotateLeft45,
      ActionRequest::RotateRight45,
      ActionRequest::RotateLeft90,
      ActionRequest::RotateRight90,
      ActionRequest::DoNothing
    };

    ActionRequest best = ActionRequest::DoNothing;
    int bestScore = -1;
    for (auto &c : cands) {
        if (c.score > bestScore ||
           (c.score == bestScore &&
            std::find(std::begin(order),std::end(order),c.act) <
            std::find(std::begin(order),std::end(order),best)))
        {
            bestScore = c.score;
            best      = c.act;
            direction_ = c.nd;
        }
    }

    return best;
}
