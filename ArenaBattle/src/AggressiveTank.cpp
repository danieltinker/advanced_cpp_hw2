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

static const std::pair<int, ActionRequest> ROTATIONS[] = {
    { -1, ActionRequest::RotateLeft45 },
    {  1, ActionRequest::RotateRight45 },
    { -2, ActionRequest::RotateLeft90 },
    {  2, ActionRequest::RotateRight90 },
};

AggressiveTank::AggressiveTank(int playerIndex, int /*tankIndex*/)
    : lastInfo_(0,0)
    , playerIndex_(playerIndex)
    , curX_(0)
    , curY_(0)
    , curDir_(playerIndex == 1 ? 2 : 6)
    , shellsLeft_(-1)
    , seenInfo_(false)
    , ticksSinceInfo_(0)
    , algoCooldown_(0)
{
    std::cerr << "DEBUG: AggressiveTank init player=" << playerIndex_
              << " dir=" << curDir_ << "\n";
}

void AggressiveTank::updateBattleInfo(BattleInfo& info) {
    auto* pInfo = dynamic_cast<MyBattleInfo*>(&info);
    if (!pInfo) {
        std::cerr << "ERROR: AggressiveTank::updateBattleInfo - unexpected info type\n";
        return;
    }
    lastInfo_ = *pInfo;
    if (shellsLeft_ < 0) shellsLeft_ = static_cast<int>(lastInfo_.shellsRemaining);
    seenInfo_ = true;
    ticksSinceInfo_ = 0;
    curX_ = lastInfo_.selfX;
    curY_ = lastInfo_.selfY;
    plan_.clear();
    std::cerr << "DEBUG: updateBattleInfo pos=(" << curX_ << "," << curY_
              << ") shells=" << shellsLeft_ << "\n";
}

bool AggressiveTank::needsInfo() {
    if (!seenInfo_) return true;
    if (++ticksSinceInfo_ >= REFRESH_INTERVAL) {
        seenInfo_ = false;
        return true;
    }
    if (algoCooldown_ > 0) {
        --algoCooldown_;
        seenInfo_ = false;
        return true;
    }
    return false;
}

ActionRequest AggressiveTank::getAction() {
    std::cerr << "DEBUG: getAction seen=" << seenInfo_
              << " ticks=" << ticksSinceInfo_
              << " cd=" << algoCooldown_
              << " plan=" << plan_.size()
              << " pos=(" << curX_ << "," << curY_
              << ") dir=" << curDir_
              << " shells=" << shellsLeft_ << "\n";

    if (needsInfo()) {
        std::cerr << "DEBUG: ->GetBattleInfo (info needed)\n";
        return ActionRequest::GetBattleInfo;
    }
    if (plan_.empty()) computePlan();

    if (!plan_.empty()) {
        auto act = plan_.front(); plan_.pop_front();
        std::cerr << "DEBUG: act=" << static_cast<int>(act) << "\n";
        switch (act) {
            case ActionRequest::RotateLeft45:  curDir_ = (curDir_ + 7) % 8; break;
            case ActionRequest::RotateRight45: curDir_ = (curDir_ + 1) % 8; break;
            case ActionRequest::RotateLeft90:  curDir_ = (curDir_ + 6) % 8; break;
            case ActionRequest::RotateRight90: curDir_ = (curDir_ + 2) % 8; break;
            case ActionRequest::MoveForward:   curX_ += DX[curDir_]; curY_ += DY[curDir_]; break;
            case ActionRequest::MoveBackward:  curX_ -= DX[curDir_]; curY_ -= DY[curDir_]; break;
            case ActionRequest::Shoot:
                --shellsLeft_;
                algoCooldown_ = SHOOT_CD;
                seenInfo_ = false;
                break;
            default: break;
        }
        return act;
    }

    std::cerr << "DEBUG: ->GetBattleInfo (fallback)\n";
    seenInfo_ = false;
    return ActionRequest::GetBattleInfo;
}

std::vector<AggressiveTank::ShootState> AggressiveTank::findShootStates() const {
    std::vector<ShootState> out;
    int rows = static_cast<int>(lastInfo_.grid.size());
    if (rows == 0) return out;
    int cols = static_cast<int>(lastInfo_.grid[0].size());

    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            for (int d = 0; d < 8; ++d) {
                int ds, wh;
                if (lineOfSight(x, y, d, ds, wh)) {
                    int cost = wh * 2 + 1;
                    if (cost <= shellsLeft_) out.push_back({{x,y,d}, wh});
                }
            }
        }
    }
    return out;
}

std::vector<ActionRequest> AggressiveTank::planPathTo(const State& target) {
    int rows = static_cast<int>(lastInfo_.grid.size()); if (rows == 0) return {};
    int cols = static_cast<int>(lastInfo_.grid[0].size());
    int total = rows * cols * 8;
    const int INF = std::numeric_limits<int>::max()/2;

    auto idx = [&](int x,int y,int d){ return (y*cols + x)*8 + d; };
    int start = idx(curX_,curY_,curDir_);
    int goal  = idx(target.x,target.y,target.dir);

    std::vector<int> g(total,INF), f(total,INF), parent(total,-1);
    std::vector<ActionRequest> via(total);
    using P = std::pair<int,int>;
    std::priority_queue<P,std::vector<P>,std::greater<>> open;

    auto h = [&](int x,int y,int d){
        int md = std::abs(x-target.x)+std::abs(y-target.y);
        int rd = std::abs(d-target.dir); rd = std::min(rd,8-rd);
        return md*MOVE_COST + rd*ROTATE_COST;
    };

    g[start]=0; f[start]=h(curX_,curY_,curDir_);
    open.push({f[start],start});

    while(!open.empty()){
        auto [fv,u]=open.top(); open.pop();
        if(u==goal) break;
        if(fv>f[u]) continue;
        int ux=(u/8)%cols, uy=(u/8)/cols, ud=u%8;
        int base=g[u];
        for(const auto &pr : ROTATIONS) {
            int delta = pr.first;
            ActionRequest act = pr.second;
            int nd = (ud + delta + 8) % 8;
            int v = idx(ux,uy,nd);
            int ng = base + ROTATE_COST;
            if (ng < g[v]) {
                g[v] = ng;
                parent[v] = u;
                via[v] = act;
                f[v] = ng + h(ux,uy,nd);
                open.push({f[v], v});
            }
        }
        int fx = ux + DX[ud], fy = uy + DY[ud];
        if (isTraversable(fx, fy)) {
            int v = idx(fx,fy,ud);
            int ng = base + MOVE_COST;
            if (ng < g[v]) {
                g[v] = ng;
                parent[v] = u;
                via[v] = ActionRequest::MoveForward;
                f[v] = ng + h(fx,fy,ud);
                open.push({f[v], v});
            }
        }
    }

    std::vector<ActionRequest> seq;
    if (parent[goal] < 0) return seq;
    for (int v = goal; v != start; v = parent[v]) seq.push_back(via[v]);
    std::reverse(seq.begin(), seq.end());
    return seq;
}

std::vector<ActionRequest> AggressiveTank::buildShootSequence(int walls) const {
    std::vector<ActionRequest> s;
    int shots = walls*2 + 1;
    while (shots-- > 0) s.push_back(ActionRequest::Shoot);
    return s;
}

std::vector<ActionRequest> AggressiveTank::fallbackRoam() const {
    int fx = curX_ + DX[curDir_], fy = curY_ + DY[curDir_];
    if (isTraversable(fx, fy)) return {ActionRequest::MoveForward};
    return {ActionRequest::RotateRight45};
}

void AggressiveTank::computePlan() {
    plan_.clear();
    auto shoots = findShootStates();
    if (shoots.empty()) {
        for (auto &a : fallbackRoam()) plan_.push_back(a);
        return;
    }
    auto best = *std::min_element(shoots.begin(), shoots.end(), [&](auto &a, auto &b){
        int ha = std::abs(a.st.x - curX_) + std::abs(a.st.y - curY_);
        int hb = std::abs(b.st.x - curX_) + std::abs(b.st.y - curY_);
        return ha < hb;
    });
    for (auto &a : planPathTo(best.st))           plan_.push_back(a);
    for (auto &a : buildShootSequence(best.walls)) plan_.push_back(a);
}

bool AggressiveTank::lineOfSight(int sx, int sy, int dir, int &ds, int &wh) const {
    ds = wh = 0;
    int rows = static_cast<int>(lastInfo_.grid.size()); if (!rows) return false;
    int cols = static_cast<int>(lastInfo_.grid[0].size());
    int x = sx, y = sy;
    while (true) {
        x += DX[dir]; y += DY[dir]; ++ds;
        if (x < 0 || x >= cols || y < 0 || y >= rows) return false;
        char c = lastInfo_.grid[y][x];
        if (c == '#') { ++wh; return false; }
        if (c == '@') return false;
        if (c == '_'||c == '.'||c == '&') continue;
        if ((playerIndex_==1 && c=='2') || (playerIndex_==2 && c=='1')) return true;
        return false;
    }
}

bool AggressiveTank::isTraversable(int x, int y) const {
    int rows = static_cast<int>(lastInfo_.grid.size()); if (!rows) return false;
    int cols = static_cast<int>(lastInfo_.grid[0].size());
    if (x < 0 || y < 0 || x >= cols || y >= rows) return false;
    char c = lastInfo_.grid[y][x];
    return (c == '_' || c == '.');
}
