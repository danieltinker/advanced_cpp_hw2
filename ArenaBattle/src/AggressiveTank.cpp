#include "AggressiveTank.h"
#include "common/ActionRequest.h"
#include <queue>
#include <vector>
#include <limits>
#include <cstring>

using namespace arena;
using namespace common;

AggressiveTank::AggressiveTank(int playerIndex, int /*tankIndex*/)
  : lastInfo_{1,1}
  , shellsLeft_(-1)
  , enemyChar_(playerIndex == 1 ? '2' : '1')
  , direction_(playerIndex == 1 ? 2 : 6)  // P1 faces right, P2 faces left
  , viewCooldown_(0)
  , justShot_(false)
{}

void AggressiveTank::updateBattleInfo(BattleInfo& baseInfo) {
    lastInfo_     = static_cast<MyBattleInfo&>(baseInfo);
    if (shellsLeft_ < 0)
        shellsLeft_ = int(lastInfo_.shellsRemaining);
}

ActionRequest AggressiveTank::getAction() {
    const int R = int(lastInfo_.rows), C = int(lastInfo_.cols);
    const int sx = int(lastInfo_.selfX), sy = int(lastInfo_.selfY);

    // (1) If we have no plan and either just shot or cooldown expired → refresh view
    if (actionQueue_.empty() && (justShot_ || viewCooldown_ <= 0)) {
        viewCooldown_ = 10;
        justShot_     = false;
        return ActionRequest::GetBattleInfo;
    }

    // (2) If we have a queued action, execute it
    if (!actionQueue_.empty()) {
        auto act = actionQueue_.front();
        actionQueue_.pop_front();
        // after shooting, force next view
        if (act == ActionRequest::Shoot) {
            justShot_     = true;
            viewCooldown_ = 0;
        } else {
            --viewCooldown_;
        }
        return act;
    }

    // (3) Plan a new BFS route to any “knockable” line‐of‐fire
    int total = R*C*8;
    std::vector<char> visited(total,0);
    std::vector<int>  parent(total,-1);
    std::vector<ActionRequest> parentAct(total,ActionRequest::DoNothing);

    auto encode = [&](int x,int y,int d){ return (y*C + x)*8 + d; };
    auto decode = [&](int id,int& x,int& y,int& d){
        d = id % 8;
        int idx = id/8;
        y = idx/C;
        x = idx%C;
    };

    int start = encode(sx,sy,direction_);
    std::queue<int> q;
    visited[start] = 1;
    parent[start]  = start;
    q.push(start);

    int target = -1, ammoNeeded = 0;
    while (!q.empty() && target<0) {
        int cur = q.front(); q.pop();
        int x,y,d;
        decode(cur,x,y,d);

        // look along direction d
        int wx=x, wy=y, walls=0;
        while (true) {
            wx += DX[d];
            wy += DY[d];
            if (wx<0||wx>=C||wy<0||wy>=R) break;
            char c = lastInfo_.grid[wy][wx];
            if (c=='#') { walls++; continue; }
            if (c==enemyChar_) {
                int cost = walls*2 + 1;
                if (cost <= shellsLeft_) {
                    target = cur;
                    ammoNeeded = cost;
                }
            }
            break;
        }
        if (target>=0) break;

        // rotate right 45°
        { int nd=(d+1)&7, id=encode(x,y,nd);
          if (!visited[id]) {
            visited[id]=1; parent[id]=cur; parentAct[id]=ActionRequest::RotateRight45; q.push(id);
          }
        }
        // rotate left 45°
        { int nd=(d+7)&7, id=encode(x,y,nd);
          if (!visited[id]) {
            visited[id]=1; parent[id]=cur; parentAct[id]=ActionRequest::RotateLeft45;  q.push(id);
          }
        }
        // move forward if empty
        { int nx=x+DX[d], ny=y+DY[d];
          if (nx>=0&&nx<C&&ny>=0&&ny<R && lastInfo_.grid[ny][nx]==' ') {
            int id=encode(nx,ny,d);
            if (!visited[id]) {
              visited[id]=1; parent[id]=cur; parentAct[id]=ActionRequest::MoveForward; q.push(id);
            }
          }
        }
    }

    // (4) If we found a valid shooting position, reconstruct the plan
    if (target>=0) {
        std::vector<ActionRequest> rev;
        for (int id=target; parent[id]!=id; id=parent[id])
            rev.push_back(parentAct[id]);
        // replay
        for (auto it=rev.rbegin(); it!=rev.rend(); ++it) {
            actionQueue_.push_back(*it);
            if (*it==ActionRequest::RotateRight45) direction_=(direction_+1)&7;
            else if (*it==ActionRequest::RotateLeft45) direction_=(direction_+7)&7;
        }
        actionQueue_.push_back(ActionRequest::Shoot);
        shellsLeft_ -= ammoNeeded;

        // pop first
        auto act = actionQueue_.front();
        actionQueue_.pop_front();
        if (act==ActionRequest::Shoot) {
            justShot_=true; viewCooldown_=0;
        } else --viewCooldown_;
        return act;
    }

    // (5) Fallback: forward if clear
    {
        int fx=sx+DX[direction_], fy=sy+DY[direction_];
        if (fx>=0&&fx<C&&fy>=0&&fy<R && lastInfo_.grid[fy][fx]==' ') {
            --viewCooldown_;
            return ActionRequest::MoveForward;
        }
    }
    // (6) otherwise rotate 90° right
    direction_=(direction_+2)&7;
    --viewCooldown_;
    return ActionRequest::MoveBackward;
}
