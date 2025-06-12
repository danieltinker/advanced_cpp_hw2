#include "AggressiveTank.h"
#include <queue>
#include <limits>
#include <algorithm>
#include <iostream>
#include <sstream>

using namespace arena;
using namespace common;

// Rotation lookup table: delta -> ActionRequest
const std::pair<int, ActionRequest> AggressiveTank::ROTATIONS[4] = {
    { -1, ActionRequest::RotateLeft45 },
    {  1, ActionRequest::RotateRight45 },
    { -2, ActionRequest::RotateLeft90 },
    {  2, ActionRequest::RotateRight90 },
};

// 8-direction movement deltas
static constexpr int DX[8] = {  0, 1, 1, 1,  0, -1, -1, -1 };
static constexpr int DY[8] = { -1,-1, 0, 1,  1,  1,  0, -1 };

AggressiveTank::AggressiveTank(int playerIndex, int /*tankIndex*/)
  : lastInfo_(0,0)
  , playerIndex_(playerIndex)
  , curX_(0), curY_(0)
  , curDir_(playerIndex==1?2:6)
  , shellsLeft_(-1)
  , seenInfo_(false)
  , ticksSinceInfo_(0)
  , algoCooldown_(0)
{
    std::cerr << "[INIT] player=" << playerIndex_ << " startDir=" << curDir_ << "\n";
}

void AggressiveTank::updateBattleInfo(common::BattleInfo& info) {
    lastInfo_ = static_cast<MyBattleInfo&>(info);
    if (shellsLeft_ < 0)
        shellsLeft_ = static_cast<int>(lastInfo_.shellsRemaining);

    seenInfo_      = true;
    ticksSinceInfo_= 0;
    curX_          = lastInfo_.selfX;
    curY_          = lastInfo_.selfY;

    int rows = lastInfo_.grid.size();
    int cols = rows ? lastInfo_.grid[0].size() : 0;
    std::cerr << "[INFO] sees " << rows << "x" << cols
              << " at (" << curX_ << "," << curY_ << ") shells="
              << shellsLeft_ << "\n";
    for (int y = 0; y < rows; ++y) {
        std::cerr << "[GRID] ";
        for (char c : lastInfo_.grid[y]) std::cerr << c;
        std::cerr << "\n";
    }
}

bool AggressiveTank::needsInfo() {
    if (!seenInfo_) return true;
    if (++ticksSinceInfo_ >= REFRESH_INTERVAL) {
        seenInfo_ = false;
        return true;
    }
    if (plan_.empty() && algoCooldown_ > 0) {
        --algoCooldown_;
        seenInfo_ = false;
        return true;
    }
    return false;
}

common::ActionRequest AggressiveTank::getAction() {
    std::cerr << "[GET_ACTION] seen=" << seenInfo_
              << " ticks=" << ticksSinceInfo_
              << " cd="    << algoCooldown_
              << " pos=(" << curX_ << "," << curY_ << ")"
              << " dir="  << curDir_
              << " planSize=" << plan_.size() << "\n";

    // Priority: any queued Info
    if (!plan_.empty() && plan_.front() == ActionRequest::GetBattleInfo) {
        plan_.pop_front();
        seenInfo_ = false;
        std::cerr << "[ACTION] ->GetBattleInfo (from plan)\n";
        return ActionRequest::GetBattleInfo;
    }
    // External Info need
    if (needsInfo()) {
        std::cerr << "[ACTION] ->GetBattleInfo (external)\n";
        return ActionRequest::GetBattleInfo;
    }

    if (plan_.empty())
        computePlan();

    // Log full plan queue
    {
        std::stringstream ss;
        ss << "[PLAN_QUEUE]";
        for (auto &a : plan_) ss << " " << static_cast<int>(a);
        std::cerr << ss.str() << "\n";
    }

    // Execute next
    auto act = plan_.front(); plan_.pop_front();
    std::cerr << "[EXECUTE] action=" << static_cast<int>(act) << "\n";

    switch (act) {
      case ActionRequest::RotateLeft45:
        curDir_ = (curDir_ + 7) % 8; break;
      case ActionRequest::RotateRight45:
        curDir_ = (curDir_ + 1) % 8; break;
      case ActionRequest::RotateLeft90:
        curDir_ = (curDir_ + 6) % 8; break;
      case ActionRequest::RotateRight90:
        curDir_ = (curDir_ + 2) % 8; break;
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
        seenInfo_     = false;
        std::cerr << "[SHOT] shellsLeft=" << shellsLeft_ << "\n";
        plan_.clear(); // drop leftover shots
        break;
      default:
        break;
    }
    return act;
}

std::vector<AggressiveTank::ShootState> AggressiveTank::findShootStates() const {
    std::vector<ShootState> out;
    int rows = lastInfo_.grid.size();
    int cols = rows ? lastInfo_.grid[0].size() : 0;
    for (int d = 0; d < 8; ++d) {
        int ds = 0, wh = 0;
        if (lineOfSight(curX_, curY_, d, ds, wh)) {
            int tx = curX_ + ds * DX[d];
            int ty = curY_ + ds * DY[d];
            if (tx >= 0 && tx < cols && ty >= 0 && ty < rows) {
                out.push_back({{tx, ty, d}, wh, ds});
                std::cerr << "[CAND] dir="<<d
                          <<" ds="<<ds
                          <<" walls="<<wh
                          <<" target=("<<tx<<","<<ty<<")\n";
            }
        }
    }
    return out;
}

std::vector<common::ActionRequest> AggressiveTank::planPathTo(const State& target) {
    int rows = lastInfo_.grid.size();
    int cols = rows ? lastInfo_.grid[0].size() : 0;
    int total = rows * cols * 8;
    const int INF = std::numeric_limits<int>::max()/2;
    auto idx = [&](int x,int y,int d){ return (y*cols + x)*8 + d; };

    int start = idx(curX_, curY_, curDir_);
    int goal  = idx(target.x, target.y, target.dir);
    std::vector<int> g(total,INF), f(total,INF), parent(total,-1);
    std::vector<ActionRequest> via(total);
    using P=std::pair<int,int>;
    std::priority_queue<P, std::vector<P>, std::greater<>> open;
    auto heur = [&](int x,int y,int d){
        int md = abs(x-target.x)+abs(y-target.y);
        int rd = abs(d-target.dir);
        rd = std::min(rd,8-rd);
        return md*MOVE_COST + rd*ROTATE_COST;
    };

    g[start] = 0;
    f[start] = heur(curX_,curY_,curDir_);
    open.push({f[start], start});

    while(!open.empty()){
        auto [fv,u] = open.top(); open.pop();
        if(u==goal) break;
        if(fv>f[u]) continue;
        int ux = (u/8) % cols;
        int uy = (u/8) / cols;
        int ud = u % 8;
        int base = g[u];
        // rotations
        for(auto &pr: ROTATIONS){
            int nd = (ud + pr.first + 8) % 8;
            int v  = idx(ux,uy,nd);
            int cost = base + (std::abs(pr.first)==2 ? ROTATE_COST*2 : ROTATE_COST);
            if(cost < g[v]){
                g[v] = cost;
                f[v] = cost + heur(ux,uy,nd);
                parent[v] = u;
                via[v]    = pr.second;
                open.push({f[v],v});
            }
        }
        // forward
        int fx = ux + DX[ud];
        int fy = uy + DY[ud];
        if(fx>=0&&fx<cols&&fy>=0&&fy<rows&&isTraversable(fx,fy)){
            int v = idx(fx,fy,ud);
            int cost = base + MOVE_COST;
            if(cost < g[v]){
                g[v] = cost;
                f[v] = cost + heur(fx,fy,ud);
                parent[v] = u;
                via[v]    = ActionRequest::MoveForward;
                open.push({f[v],v});
            }
        }
    }
    std::vector<ActionRequest> seq;
    if(parent[goal] < 0){
        std::cerr << "[PATH] no path to ("<< target.x <<","<< target.y <<") dir="<< target.dir <<"\n";
        return seq;
    }
    for(int v=goal; v!=start; v = parent[v])
        seq.push_back(via[v]);
    std::reverse(seq.begin(), seq.end());
    std::cerr << "[PATH] to ("<< target.x <<","<< target.y <<") dir="<< target.dir <<" len="<< seq.size() <<"\n";
    return seq;
}

std::vector<common::ActionRequest> AggressiveTank::buildShootSequence(int walls) const {
    std::vector<ActionRequest> s;
    int shots = walls*2 + 1;
    std::cerr << "[SEQ] building " << shots << " shots\n";
    while(shots-- > 0)
        s.push_back(ActionRequest::Shoot);
    return s;
}

std::vector<common::ActionRequest> AggressiveTank::fallbackRoam() const {
    int rows = lastInfo_.grid.size();
    int cols = rows ? lastInfo_.grid[0].size() : 0;
    int fx = curX_ + DX[curDir_];
    int fy = curY_ + DY[curDir_];
    if(fx>=0&&fx<cols&&fy>=0&&fy<rows
       && lastInfo_.grid[fy][fx] != '@'
       && isTraversable(fx,fy))
    {
        std::cerr << "[ROAM] MoveForward to ("<<fx<<","<<fy<<")\n";
        return { ActionRequest::MoveForward };
    }
    std::cerr << "[ROAM] RotateRight45 (blocked)\n";
    return { ActionRequest::RotateRight45 };
}

void AggressiveTank::computePlan() {
    plan_.clear();
    std::cerr << "[COMPUTE] computePlan()\n";

    int rows = lastInfo_.grid.size();
    int cols = rows ? lastInfo_.grid[0].size() : 0;
    char eChar = (playerIndex_ == 1 ? '2' : '1');

    // 1) collect direct-shoot candidates
    auto shoots = findShootStates();
    std::vector<ShootState> real;
    for(auto &ss: shoots){
        if(lastInfo_.grid[ss.st.y][ss.st.x] == eChar){
            real.push_back(ss);
            std::cerr << "[VALID] enemy at ("<< ss.st.x <<","<< ss.st.y <<")\n";
        }
    }

    // 2) if none, chase nearest enemy
    if(real.empty()){
        std::vector<State> enemies;
        for(int y=0; y<rows; ++y)
            for(int x=0; x<cols; ++x)
                if(lastInfo_.grid[y][x] == eChar)
                    enemies.push_back({x,y,curDir_});
        if(!enemies.empty()){
            auto tgt = *std::min_element(
                enemies.begin(), enemies.end(),
                [&](auto &a, auto &b){
                    int da = abs(a.x-curX_) + abs(a.y-curY_);
                    int db = abs(b.x-curX_) + abs(b.y-curY_);
                    return da < db;
                }
            );
            std::cerr << "[CHASE] nearest enemy at ("<< tgt.x <<","<< tgt.y <<")\n";
            auto path = planPathTo(tgt);
            if(!path.empty()){
                plan_.insert(plan_.end(), path.begin(), path.end());
                return;
            }
        }
        // fallback roam if no path
        auto roam = fallbackRoam();
        plan_.insert(plan_.end(), roam.begin(), roam.end());
        return;
    }

    // 3) pick best direct-shoot
    auto best = *std::min_element(
        real.begin(), real.end(),
        [&](auto &a, auto &b){
            int da = abs(a.st.x-curX_) + abs(a.st.y-curY_);
            int db = abs(b.st.x-curX_) + abs(b.st.y-curY_);
            return da < db;
        }
    );
    std::cerr << "[BEST] dir="<< best.st.dir
              <<" at ("<< best.st.x <<","<< best.st.y <<")"
              <<" ds="<< best.ds <<" walls="<< best.walls <<"\n";

    // 4) move into firing cell
    auto path = planPathTo(best.st);
    plan_.insert(plan_.end(), path.begin(), path.end());

    // 5) refresh
    plan_.push_back(ActionRequest::GetBattleInfo);

    // 6) simulate heading & rotate
    int simDir = curDir_;
    for(auto &act: path){
        if(act == ActionRequest::RotateLeft45)  simDir = (simDir+7)%8;
        if(act == ActionRequest::RotateRight45) simDir = (simDir+1)%8;
        if(act == ActionRequest::RotateLeft90)  simDir = (simDir+6)%8;
        if(act == ActionRequest::RotateRight90) simDir = (simDir+2)%8;
    }
    int diff = (best.st.dir - simDir + 8) % 8;
    std::cerr << "[SIM] simDir="<< simDir <<" diff="<< diff <<"\n";
    if(diff == 4){
        plan_.push_back(ActionRequest::RotateRight90);
        plan_.push_back(ActionRequest::RotateRight90);
    } else if(diff <= 4){
        for(int i=0;i<diff;++i)
            plan_.push_back(ActionRequest::RotateRight45);
    } else {
        for(int i=0;i<8-diff;++i)
            plan_.push_back(ActionRequest::RotateLeft45);
    }

    // 7) fire
    auto seq = buildShootSequence(best.walls);
    plan_.insert(plan_.end(), seq.begin(), seq.end());
}

bool AggressiveTank::lineOfSight(int sx,int sy,int dir,int &ds,int &wh) const {
    ds = wh = 0;
    int rows = lastInfo_.grid.size(), cols = rows?lastInfo_.grid[0].size():0;
    int x = sx, y = sy;
    while(true){
        x += DX[dir]; y += DY[dir];
        if(x<0||x>=cols||y<0||y>=rows) return false;
        ++ds;
        char c = lastInfo_.grid[y][x];
        if(c == '#') { ++wh; continue; }
        if(c == '@')   continue;
        if(c == '_'||c=='.'||c=='%'||c==' ') continue;
        if((playerIndex_==1&&c=='1')||(playerIndex_==2&&c=='2')) return false;
        if((playerIndex_==1&&c=='2')||(playerIndex_==2&&c=='1')) return true;
        return false;
    }
}

bool AggressiveTank::isTraversable(int x,int y) const {
    int rows = lastInfo_.grid.size();
    int cols = rows?lastInfo_.grid[0].size():0;
    if(x<0||x>=cols||y<0||y>=rows) return false;
    char c = lastInfo_.grid[y][x];
    return c=='_'||c=='.'||c=='%'||c==' ';
}
