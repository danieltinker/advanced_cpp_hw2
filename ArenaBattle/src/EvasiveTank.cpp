// src/EvasiveTank.cpp
#include "EvasiveTank.h"
#include "common/ActionRequest.h"
#include <limits>
#include <queue>

using namespace arena;
using namespace common;

EvasiveTank::EvasiveTank(int playerIndex, int /*tankIndex*/)
  : lastInfo_{1,1}
  , direction_(playerIndex == 1 ? 6 : 2)
  , shellsLeft_(-1)
  , needView_(true)
{}

void EvasiveTank::updateBattleInfo(BattleInfo& baseInfo) {
    lastInfo_ = static_cast<MyBattleInfo&>(baseInfo);
    if (shellsLeft_ < 0) {
        shellsLeft_ = int(lastInfo_.shellsRemaining);
    }
}

bool EvasiveTank::isFree(int x, int y) const {
    if (x < 0 || x >= int(lastInfo_.cols) ||
        y < 0 || y >= int(lastInfo_.rows)) return false;
    char c = lastInfo_.grid[y][x];
    return c != '#' && c != '@' && c != '1' && c != '2';
}

ActionRequest EvasiveTank::getAction() {
    // (1) always get fresh view when flagged
    if (needView_) {
        needView_ = false;
        return ActionRequest::GetBattleInfo;
    }
    needView_ = true;

    int sx = int(lastInfo_.selfX);
    int sy = int(lastInfo_.selfY);
    int R  = int(lastInfo_.rows);
    int C  = int(lastInfo_.cols);

    // (2) scan for any shell '*' up to two steps away
    struct Threat { int dir, dist; };
    std::vector<Threat> threats;
    for (int d = 0; d < 8; ++d) {
        int dx = DX[d], dy = DY[d];
        for (int step = 1; step <= 2; ++step) {
            int nx = sx + dx*step;
            int ny = sy + dy*step;
            if (nx<0||nx>=C||ny<0||ny>=R) break;
            if (lastInfo_.grid[ny][nx] == '*') {
                threats.push_back({d, step});
                break;
            }
        }
    }
    if (!threats.empty()) {
        // (3) find best escape direction: maximize distance from nearest threat
        int bestDir=-1, bestMinDist=-1;
        for (int cand = 0; cand < 8; ++cand) {
            int dx = DX[cand], dy = DY[cand];
            int x1 = sx+dx, y1=sy+dy;
            if (!isFree(x1,y1)) continue;
            // compute minimal projection of all threats onto this dir
            int minDist = std::numeric_limits<int>::max();
            for (auto& t : threats) {
                // project threat vector onto cand
                int proj = t.dist * (DX[t.dir]*dx + DY[t.dir]*dy);
                // use absolute or just treat any non-positive as close
                minDist = std::min(minDist, proj>0 ? proj : 0);
            }
            if (minDist > bestMinDist) {
                bestMinDist = minDist;
                bestDir     = cand;
            }
        }
        if (bestDir >= 0) {
            // rotate toward bestDir
            int diff = (bestDir - direction_ + 8) & 7;
            if (diff==1) {
                direction_ = (direction_+1)&7;
                return ActionRequest::RotateRight45;
            }
            if (diff==7) {
                direction_ = (direction_+7)&7;
                return ActionRequest::RotateLeft45;
            }
            if (diff==2) {
                direction_ = (direction_+2)&7;
                return ActionRequest::RotateRight90;
            }
            if (diff==6) {
                direction_ = (direction_+6)&7;
                return ActionRequest::RotateLeft90;
            }
            // if already facing roughly, just move backward
            return ActionRequest::MoveBackward;
        }
    }

    // (4) blocked ahead?   handle wall/mine/tank
    int fx = sx + DX[direction_], fy = sy + DY[direction_];
    if (!isFree(fx,fy)) {
        // pick best rotation left or right
        int leftDir  = (direction_+7)&7;
        int rightDir = (direction_+1)&7;
        bool freeL = isFree(sx+DX[leftDir], sy+DY[leftDir]);
        bool freeR = isFree(sx+DX[rightDir],sy+DY[rightDir]);
        if (freeR && !freeL) {
            direction_ = rightDir;
            return ActionRequest::RotateRight90;
        }
        if (freeL && !freeR) {
            direction_ = leftDir;
            return ActionRequest::RotateLeft90;
        }
        // if both or neither free, default right
        direction_ = rightDir;
        return ActionRequest::RotateRight90;
    }

    // (5) default: move forward
    return ActionRequest::MoveForward;
}
