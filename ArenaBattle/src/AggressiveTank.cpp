#include "AggressiveTank.h"
#include "ActionRequest.h"
#include "utils.h"
#include <queue>
#include <vector>
#include <limits>
#include <iostream>

using namespace arena;
using namespace common;

// 8‐direction deltas
static constexpr int DX[8] = { 0, 1, 1, 1, 0, -1, -1, -1 };
static constexpr int DY[8] = {-1,-1, 0, 1, 1,  1,  0, -1 };

AggressiveTank::AggressiveTank(int playerIndex, int /*tankIndex*/)
  : lastInfo_{1,1}
  , shellsLeft_(-1)
  , enemyChar_(playerIndex==1?'2':'1')
  , direction_(playerIndex==1?2:6)
{}

void AggressiveTank::updateBattleInfo(BattleInfo& info) {
    lastInfo_ = static_cast<MyBattleInfo&>(info);
    // Only initialize shellsLeft_ the very first time:
    if (shellsLeft_ < 0) {
        shellsLeft_ = int(lastInfo_.shellsRemaining);
        std::cerr << "[Aggressive] Initial shellsLeft=" << shellsLeft_ << "\n";
    }
    seenInfo_ = true;
}


int AggressiveTank::encode(int x,int y,int d) const {
    return (y * lastInfo_.cols + x) * 8 + d;
}

void AggressiveTank::decode(int code,int& x,int& y,int& d) const {
    d = code % 8;
    int idx = code / 8;
    y = idx / lastInfo_.cols;
    x = idx % lastInfo_.cols;
}

common::ActionRequest AggressiveTank::getAction() {
    std::cerr << "[Aggressive] Decision: pos=("
              << lastInfo_.selfX<<","<<lastInfo_.selfY
              <<") dir="<<direction_
              <<" shells="<<shellsLeft_
              <<" cd="<<algoShootCooldown_<<"\n";

    // 1) If no info yet → get it
    if (!seenInfo_) {
        std::cerr<<" → Requesting GetBattleInfo\n";
        return ActionRequest::GetBattleInfo;
    }

    // 1b) If we're in shoot cooldown and no plan, reposition instead of planning shoot
    if (algoShootCooldown_ > 0 && plan_.empty()) {
        --algoShootCooldown_;
        std::cerr<<" → In shoot cooldown, repositioning\n";
        // simple reposition: rotate 180° to face the enemy
        direction_ = (direction_ + 4) & 7;
        return ActionRequest::RotateRight90;  // two of these over two turns = 180°
    }

    // 2) If there’s a queued plan, execute it
    if (!plan_.empty()) {
        auto act = plan_.front();
        plan_.pop_front();
        std::cerr<<" → Executing plan action "<<actionToString(act)<<"\n";
        if (act == ActionRequest::Shoot) {
            // start our algo‐level cooldown
            algoShootCooldown_ = ALG_SHOOT_CD;
            seenInfo_          = false;
        }
        return act;
    }

    // 3) BFS‐plan to nearest shoot‐through‐walls vantage
    {
        std::cerr<<" → BFS planning\n";
        int R = lastInfo_.rows, C = lastInfo_.cols;
        int sx = lastInfo_.selfX, sy = lastInfo_.selfY;

        int total = R*C*8;
        std::vector<char>          visited(total,0);
        std::vector<int>           parent(total,-1);
        std::vector<ActionRequest> parentAct(total,ActionRequest::DoNothing);
        std::queue<int>            q;

        int start = encode(sx,sy,direction_);
        visited[start] = 1;
        parent[start]  = start;
        q.push(start);

        int bestState = -1, bestCost = std::numeric_limits<int>::max();

        while (!q.empty() && bestState<0) {
            int cur = q.front(); q.pop();
            int x,y,d; decode(cur,x,y,d);

            // scan along d through empties and walls
            int wx=x, wy=y, walls=0;
            while (true) {
                wx += DX[d];
                wy += DY[d];
                if (wx<0||wx>=C||wy<0||wy>=R) break;
                char c = lastInfo_.grid[wy][wx];
                if (c=='@') { break; }       // mine blocks
                if (c=='#') { walls++; continue; }
                if (c==enemyChar_) {
                    int cost = walls*2 + 1;
                    std::cerr<<"    Enemy at ("<<wx<<","<<wy<<") cost="<<cost<<"\n";
                    if (cost <= shellsLeft_) {
                        bestState = cur;
                        bestCost  = cost;
                    }
                }
                // empty → keep scanning past it
                continue;
            }
            if (bestState>=0) break;

            // expand: R45
            { int nd=(d+1)&7, id=encode(x,y,nd);
              if (!visited[id]) {
                  visited[id]=1; parent[id]=cur;
                  parentAct[id]=ActionRequest::RotateRight45;
                  q.push(id);
              }
            }
            // expand: L45
            { int nd=(d+7)&7, id=encode(x,y,nd);
              if (!visited[id]) {
                  visited[id]=1; parent[id]=cur;
                  parentAct[id]=ActionRequest::RotateLeft45;
                  q.push(id);
              }
            }
            // expand: MoveForward
            { int nx=x+DX[d], ny=y+DY[d];
              if (nx>=0&&nx<C&&ny>=0&&ny<R
                  && lastInfo_.grid[ny][nx]==' ')
              {
                  int id=encode(nx,ny,d);
                  if (!visited[id]) {
                      visited[id]=1; parent[id]=cur;
                      parentAct[id]=ActionRequest::MoveForward;
                      q.push(id);
                  }
              }
            }
        }

        if (bestState < 0) {
            std::cerr<<" → No shooting vantage → fallback GetBattleInfo\n";
            seenInfo_ = false;
            return ActionRequest::GetBattleInfo;
        }

        // reconstruct plan + Shoot
        std::vector<ActionRequest> rev;
        for (int st=bestState; parent[st]!=st; st=parent[st])
            rev.push_back(parentAct[st]);
        std::cerr<<" → Enqueue plan of "<<rev.size()<<"+Shoot steps\n";
        for (auto it=rev.rbegin(); it!=rev.rend(); ++it) {
            std::cerr<<"    step "<<actionToString(*it)<<"\n";
            plan_.push_back(*it);
            if (*it==ActionRequest::RotateRight45)
                direction_=(direction_+1)&7;
            else if (*it==ActionRequest::RotateLeft45)
                direction_=(direction_+7)&7;
        }
        plan_.push_back(ActionRequest::Shoot);
        shellsLeft_ -= bestCost;
        std::cerr<<"    -> shellsLeft now="<<shellsLeft_<<"\n";
    }

    // 4) Execute the first step
    if (!plan_.empty()) {
        auto act = plan_.front();
        plan_.pop_front();
        std::cerr<<" → Exec "<<actionToString(act)<<"\n";
        if (act==ActionRequest::Shoot) {
            algoShootCooldown_ = ALG_SHOOT_CD;
            seenInfo_          = false;
        }
        return act;
    }

    // fallback safety
    std::cerr<<" → Safety fallback GetBattleInfo\n";
    seenInfo_ = false;
    return ActionRequest::GetBattleInfo;
}
