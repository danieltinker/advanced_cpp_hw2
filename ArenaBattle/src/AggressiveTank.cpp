// AggressiveTank.cpp
#include "AggressiveTank.h"
#include <queue>
#include <limits>
#include <algorithm>
#include <iostream>

using namespace arena;
using namespace common;

static constexpr int DX[8] = {  0, 1, 1, 1,  0, -1, -1, -1 };
static constexpr int DY[8] = { -1,-1, 0, 1,  1,  1,  0, -1 };

AggressiveTank::AggressiveTank(int playerIndex, int /*tankIndex*/)
  : lastInfo_(0,0),
    curDir_(playerIndex == 1 ? 2 : 6),
    playerIndex_(playerIndex)
{
    std::cerr << "DEBUG: AggressiveTank init player=" << playerIndex
              << " dir=" << curDir_ << "\n";
}

void AggressiveTank::updateBattleInfo(BattleInfo& info) {
    lastInfo_       = static_cast<MyBattleInfo&>(info);
    if (shellsLeft_ < 0) shellsLeft_ = (int)lastInfo_.shellsRemaining;
    seenInfo_       = true;
    ticksSinceInfo_ = 0;
    curX_           = (int)lastInfo_.selfX;
    curY_           = (int)lastInfo_.selfY;
    plan_.clear();
    std::cerr << "DEBUG: updateBattleInfo pos=("
              << curX_ << "," << curY_ << ") shells="
              << shellsLeft_ << "\n";
}
common::ActionRequest AggressiveTank::getAction() {
    std::cerr << "DEBUG: getAction seen=" << seenInfo_
              << " ticks="  << ticksSinceInfo_
              << " cd="     << algoCooldown_
              << " plan="   << plan_.size()
              << " pos=("   << curX_ << "," << curY_
              << ") dir="  << curDir_
              << " shells="<< shellsLeft_ << "\n";

    // 1) Need info?
    if (!seenInfo_) {
        std::cerr << "DEBUG: ->GetBattleInfo no info\n";
        return ActionRequest::GetBattleInfo;
    }
    // 2) Stale info?
    if (++ticksSinceInfo_ >= REFRESH_INTERVAL) {
        seenInfo_ = false;
        std::cerr << "DEBUG: ->GetBattleInfo refresh\n";
        return ActionRequest::GetBattleInfo;
    }
    // 3) Post-shoot cooldown: fetch new info & recompute plan
    if (algoCooldown_ > 0) {
        --algoCooldown_;
        seenInfo_ = false;  
        std::cerr << "DEBUG: ->GetBattleInfo post-shoot cd\n";
        return ActionRequest::GetBattleInfo;
    }
    // 4) Build plan if needed
    if (plan_.empty()) {
        std::cerr << "DEBUG: computing plan\n";
        computePlan();
    }
    // 5) Execute next step of plan
    if (!plan_.empty()) {
        auto act = plan_.front(); plan_.pop_front();
        std::cerr << "DEBUG: act=" << static_cast<int>(act) << "\n";
        switch (act) {
            case ActionRequest::RotateLeft45:
                curDir_ = (curDir_ + 7) % 8;
                break;
            case ActionRequest::RotateRight45:
                curDir_ = (curDir_ + 1) % 8;
                break;
            case ActionRequest::MoveForward:
                curX_ += DX[curDir_];
                curY_ += DY[curDir_];
                break;
            case ActionRequest::MoveBackward:
                curX_ -= DX[curDir_];
                curY_ -= DY[curDir_];
                break;
            case ActionRequest::Shoot:
                --shellsLeft_;
                algoCooldown_ = SHOOT_CD;
                // force a data refresh next turn
                seenInfo_ = false;
                break;
            default:
                break;
        }
        return act;
    }
    // 6) Fallback
    seenInfo_ = false;
    std::cerr << "DEBUG: ->GetBattleInfo fallback\n";
    return ActionRequest::GetBattleInfo;
}
void AggressiveTank::computePlan() {
    std::cerr << "DEBUG: computePlan pos=("
              << curX_ << "," << curY_
              << ") dir="   << curDir_
              << " shells=" << shellsLeft_
              << "\n";

    int rows = (int)lastInfo_.rows;
    int cols = (int)lastInfo_.cols;
    int total = rows * cols * 8;
    const int INF = std::numeric_limits<int>::max();

    std::vector<int> dist(total, INF), parent(total, -1);
    std::vector<common::ActionRequest> via(total);
    std::priority_queue<std::pair<int,int>,
        std::vector<std::pair<int,int>>,
        std::greater<>> pq;

    int start = (curY_ * cols + curX_) * 8 + curDir_;
    dist[start] = 0;
    pq.push({0, start});

    int bestU = -1, bestT = INF;
    while (!pq.empty()) {
        auto [t,u] = pq.top(); pq.pop();
        if (t > dist[u]) continue;

        int ux = (u / 8) % cols;
        int uy = (u / 8) / cols;
        int ud = u % 8;

        // look ahead in direction ud
        int walls = 0;
        bool vis = false;
        int tx = ux, ty = uy;
        while (true) {
            tx += DX[ud];
            ty += DY[ud];
            if (tx < 0 || tx >= cols || ty < 0 || ty >= rows)
                break;  // off‐map

            char c = lastInfo_.grid[ty][tx];
            if (c == '#') {        // wall: block vision, count it
                walls++;
                continue;
            }
            if (c == '@') {        // mine: block but don't count
                break;
            }
            // empty ground in our grid can be '.' or '_'
            if (c == '.' || c == '_') {
                continue;
            }
            if (c == '&') {        // yourself: transparent
                continue;
            }
            // friendly tank: block
            if ((playerIndex_ == 1 && c == '1') ||
                (playerIndex_ == 2 && c == '2'))
            {
                break;
            }
            // at this point only the *other* player's digit remains
            if ((playerIndex_ == 1 && c == '2') ||
                (playerIndex_ == 2 && c == '1'))
            {
                vis = true;
            }
            break;
        }

        if (vis) {
            int cost = walls * 2 + 1;  // 2 shells per wall +1 to hit
            if (cost <= shellsLeft_) {
                int tt = t + ROTATE_COST + 1;
                if (tt < bestT) {
                    bestT = tt;
                    bestU = u;
                    std::cerr << "DEBUG: found u=" << u
                              << " t=" << tt
                              << " walls=" << walls << "\n";
                }
            }
            break;
        }

        // explore rotations
        for (int delta : { -1, 1, -2, 2 }) {
            int nd = (ud + delta + 8) % 8;
            int v  = (uy * cols + ux) * 8 + nd;
            int nc = t + ROTATE_COST;
            if (nc < dist[v]) {
                dist[v]   = nc;
                parent[v] = u;
                via[v]    = (delta == -1
                             ? ActionRequest::RotateLeft45
                             : delta == 1
                               ? ActionRequest::RotateRight45
                               : delta == -2
                                 ? ActionRequest::RotateLeft90
                                 : ActionRequest::RotateRight90);
                pq.push({nc, v});
            }
        }

        // explore forward move
        int fx = ux + DX[ud], fy = uy + DY[ud];
        if (isTraversable(fx, fy)) {
            int v  = (fy * cols + fx) * 8 + ud;
            int nc = t + MOVE_COST;
            if (nc < dist[v]) {
                dist[v]   = nc;
                parent[v] = u;
                via[v]    = ActionRequest::MoveForward;
                pq.push({nc, v});
            }
        }
    }

    if (bestU < 0) {
        std::cerr << "DEBUG: no shoot state\n";
        return;
    }

    // reconstruct and enqueue path
    std::vector<common::ActionRequest> seq;
    for (int v = bestU; v != start; v = parent[v])
        seq.push_back(via[v]);
    std::reverse(seq.begin(), seq.end());
    std::cerr << "DEBUG: seq:";
    for (auto &a : seq) std::cerr << " " << static_cast<int>(a);
    std::cerr << "\n";
    for (auto &a : seq) plan_.push_back(a);

    // enqueue Shoot actions
    int ds, wh;
    if (lineOfSight((bestU/8) % cols,
                    (bestU/8) / cols,
                    bestU % 8,
                    ds, wh))
    {
        std::cerr << "DEBUG: shoot walls=" << wh
                  << " shots=" << (wh*2 + 1) << "\n";
        for (int i = 0; i < wh*2; ++i)
            plan_.push_back(ActionRequest::Shoot);
        plan_.push_back(ActionRequest::Shoot);
    }
}


bool AggressiveTank::lineOfSight(int sx,int sy,int dir,int& ds,int& wh) const {
    ds = wh = 0;
    int rows = (int)lastInfo_.rows;
    int cols = (int)lastInfo_.cols;
    int x = sx, y = sy;
    while (true) {
        x += DX[dir]; y += DY[dir];
        ++ds;
        if (x < 0 || x >= cols || y < 0 || y >= rows)
            return false;
        char c = lastInfo_.grid[y][x];
        if (c == '#') { ++wh; return false; }
        if (c == '@') return false;
        if (c == '_') continue;
        if (c == '&') continue;
        if ((playerIndex_==1 && c=='2') ||
            (playerIndex_==2 && c=='1'))
            return true;
        return false;
    }
}

bool AggressiveTank::isTraversable(int x,int y) const {
    int rows = (int)lastInfo_.rows;
    int cols = (int)lastInfo_.cols;
    if (x < 0 || x >= cols || y < 0 || y >= rows)
        return false;
    return lastInfo_.grid[y][x] == '_';
}
