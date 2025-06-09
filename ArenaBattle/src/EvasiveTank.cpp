// src/EvasiveTank.cpp
#include "EvasiveTank.h"
#include "common/ActionRequest.h"
#include <limits>
#include <cmath>

using namespace arena;
using namespace common;

EvasiveTank::EvasiveTank(int playerIndex, int /*tankIndex*/)
  : lastInfo_{1,1},
    direction_(playerIndex == 1 ? 2 : 6),  // P1 faces right(2), P2 left(6)
    needView_(true)
{}

void EvasiveTank::updateBattleInfo(BattleInfo& baseInfo) {
    lastInfo_   = static_cast<MyBattleInfo&>(baseInfo);
}

ActionRequest EvasiveTank::getAction() {
    const int R = int(lastInfo_.rows), C = int(lastInfo_.cols);
    const int sx = int(lastInfo_.selfX), sy = int(lastInfo_.selfY);

    // 1) Always fetch fresh view first
    if (needView_) {
        needView_ = false;
        return ActionRequest::GetBattleInfo;
    }
    needView_ = true;

    // 2) Gather shell positions
    std::vector<std::pair<int,int>> shells;
    for (int y = 0; y < R; ++y) {
        for (int x = 0; x < C; ++x) {
            if (lastInfo_.grid[y][x] == '*')
                shells.emplace_back(x,y);
        }
    }

    // 3) Enumerate candidate actions & score safety
    struct Cand { ActionRequest act; int nx, ny, nd; int safety; };
    std::vector<Cand> cands;
    cands.reserve(7);

    auto torDist = [&](int x, int y, int sx, int sy){
        int dx = abs(x - sx); dx = std::min(dx, C - dx);
        int dy = abs(y - sy); dy = std::min(dy, R - dy);
        int cheb = std::max(dx,dy);
        return (cheb + 1) / 2;  // shells move 2 cells/turn
    };

    auto scoreSafety = [&](int nx, int ny){
        int best = std::numeric_limits<int>::max();
        for (auto &sh : shells) {
            int d = torDist(nx,ny, sh.first, sh.second);
            best = std::min(best, d);
        }
        // if no shells, super safe
        return shells.empty() ? std::numeric_limits<int>::max() : best;
    };

    // Helper to push a candidate
    auto tryCand = [&](ActionRequest act, int nx, int ny, int nd){
        // Check legality: if move, ensure inbounds and not WALL/MINE
        if (act == ActionRequest::MoveForward ||
            act == ActionRequest::MoveBackward)
        {
            if (nx < 0 || nx >= C || ny < 0 || ny >= R)
                return;
            char c = lastInfo_.grid[ny][nx];
            if (c == '#' || c == '@') return;
        }
        int saf = scoreSafety(nx,ny);
        cands.push_back({act,nx,ny,nd,saf});
    };

    // a) MoveForward
    {
        int nx = sx + DX[direction_], ny = sy + DY[direction_];
        tryCand(ActionRequest::MoveForward, nx, ny, direction_);
    }
    // b) MoveBackward
    {
        int d2 = (direction_ + 4) & 7;
        int nx = sx + DX[d2], ny = sy + DY[d2];
        tryCand(ActionRequest::MoveBackward, nx, ny, direction_);
    }
    // c) Rotations
    tryCand(ActionRequest::RotateLeft90,  sx, sy, (direction_+6)&7);
    tryCand(ActionRequest::RotateRight90, sx, sy, (direction_+2)&7);
    tryCand(ActionRequest::RotateLeft45,  sx, sy, (direction_+7)&7);
    tryCand(ActionRequest::RotateRight45, sx, sy, (direction_+1)&7);
    // d) DoNothing
    tryCand(ActionRequest::DoNothing, sx, sy, direction_);

    // 4) Pick the candidate with highest safety
    // Tie‐break order: (MoveBackward, MoveForward, Rot45L, Rot45R, Rot90L, Rot90R, DoNothing)
    static const ActionRequest order[] = {
      ActionRequest::MoveBackward,
      ActionRequest::MoveForward,
      ActionRequest::RotateLeft45,
      ActionRequest::RotateRight45,
      ActionRequest::RotateLeft90,
      ActionRequest::RotateRight90,
      ActionRequest::DoNothing
    };
    ActionRequest bestAct = ActionRequest::DoNothing;
    int bestSafety = -1;
    for (auto &c : cands) {
        if (c.safety > bestSafety ||
           (c.safety == bestSafety &&
            std::find(std::begin(order),std::end(order),c.act) <
            std::find(std::begin(order),std::end(order),bestAct)))
        {
            bestSafety = c.safety;
            bestAct    = c.act;
            direction_ = c.nd;  // update facing for next turn
        }
    }

    return bestAct;
}
