#include "AggressiveTank.h"
#include <queue>
#include <limits>
#include <algorithm>
#include <iostream>

using namespace arena;
using namespace common;

// Delta-to-Action lookup for rotations
const std::pair<int, ActionRequest> AggressiveTank::ROTATIONS[4] = {
    { -1, ActionRequest::RotateLeft45 },
    {  1, ActionRequest::RotateRight45 },
    { -2, ActionRequest::RotateLeft90 },
    {  2, ActionRequest::RotateRight90 },
};

// Direction vectors
static constexpr int DX[8] = {  0, 1, 1, 1,  0, -1, -1, -1 };
static constexpr int DY[8] = { -1,-1, 0, 1,  1,  1,  0, -1 };

AggressiveTank::AggressiveTank(int playerIndex, int /*tankIndex*/)
  : lastInfo_(0,0),
    playerIndex_(playerIndex),
    curX_(0), curY_(0),
    curDir_(playerIndex==1?2:6),
    shellsLeft_(-1),
    seenInfo_(false),
    ticksSinceInfo_(0),
    algoCooldown_(0)
{
    std::cerr << "DEBUG: AggressiveTank init player="<<playerIndex_
              <<" dir="<<curDir_<<"\n";
}

void AggressiveTank::updateBattleInfo(BattleInfo& info) {
    auto* p = dynamic_cast<MyBattleInfo*>(&info);
    if(!p) {
        std::cerr<<"ERROR: updateBattleInfo - unexpected type\n";
        return;
    }
    lastInfo_ = *p;
    int rows = (int)lastInfo_.grid.size();
    int cols = rows? (int)lastInfo_.grid[0].size() : 0;
    std::cerr<<"DEBUG: Fresh BattleInfo grid ("<<rows<<"x"<<cols<<")\n";
    for(int y=0;y<rows;++y){
      std::cerr<<"  ";
      for(char c:lastInfo_.grid[y]) std::cerr<<c;
      std::cerr<<"\n";
    }
    if(shellsLeft_<0) shellsLeft_ = (int)lastInfo_.shellsRemaining;
    seenInfo_ = true;
    ticksSinceInfo_ = 0;
    curX_ = lastInfo_.selfX;
    curY_ = lastInfo_.selfY;
    plan_.clear();
    std::cerr<<"DEBUG: updateBattleInfo pos=("
             <<curX_<<","<<curY_<<") shells="<<shellsLeft_<<"\n";
}

bool AggressiveTank::needsInfo() {
    if(!seenInfo_) return true;
    if(++ticksSinceInfo_ >= REFRESH_INTERVAL){
        seenInfo_ = false;
        return true;
    }
    if(plan_.empty() && algoCooldown_>0){
        --algoCooldown_;
        seenInfo_ = false;
        return true;
    }
    return false;
}

ActionRequest AggressiveTank::getAction() {
    std::cerr<<"DEBUG: getAction seen="<<seenInfo_
             <<" ticks="<<ticksSinceInfo_
             <<" cd="<<algoCooldown_
             <<" plan="<<plan_.size()
             <<" pos=("<<curX_<<","<<curY_<<")"
             <<" dir="<<curDir_
             <<" shells="<<shellsLeft_<<"\n";

    if(needsInfo()){
        std::cerr<<"DEBUG: ->GetBattleInfo\n";
        return ActionRequest::GetBattleInfo;
    }
    if(plan_.empty()) computePlan();

    if(!plan_.empty()){
        auto act = plan_.front();
        plan_.pop_front();
        std::cerr<<"DEBUG: act="<<static_cast<int>(act)<<"\n";
        switch(act){
          case ActionRequest::RotateLeft45:
            curDir_ = (curDir_+7)%8; break;
          case ActionRequest::RotateRight45:
            curDir_ = (curDir_+1)%8; break;
          case ActionRequest::RotateLeft90:
            curDir_ = (curDir_+6)%8; break;
          case ActionRequest::RotateRight90:
            curDir_ = (curDir_+2)%8; break;
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
            std::cerr<<"DEBUG: shot fired, clearing plan\n";
            seenInfo_ = false;
            plan_.clear();
            break;
          default: break;
        }
        return act;
    }

    std::cerr<<"DEBUG: ->GetBattleInfo (fallback)\n";
    seenInfo_ = false;
    return ActionRequest::GetBattleInfo;
}

std::vector<AggressiveTank::ShootState> AggressiveTank::findShootStates() const {
    std::vector<ShootState> out;
    if(lastInfo_.grid.empty()) return out;
    for(int d=0;d<8;++d){
        int ds=0, wh=0;
        if(lineOfSight(curX_,curY_,d,ds,wh))
            out.push_back({{curX_,curY_,d},wh,ds});
    }
    return out;
}

std::vector<common::ActionRequest>
AggressiveTank::planPathTo(const State& target)
{
    int rows = (int)lastInfo_.grid.size();
    if(!rows) return {};
    int cols = (int)lastInfo_.grid[0].size();
    int total = rows*cols*8;
    const int INF = std::numeric_limits<int>::max()/2;
    auto idx = [&](int x,int y,int d){ return (y*cols + x)*8 + d; };
    int start = idx(curX_,curY_,curDir_);
    int goal  = idx(target.x,target.y,target.dir);

    std::vector<int> g(total,INF), f(total,INF), parent(total,-1);
    std::vector<ActionRequest> via(total);
    using P = std::pair<int,int>;
    std::priority_queue<P,std::vector<P>,std::greater<>> open;

    auto heuristic = [&](int x,int y,int d){
        int md = abs(x-target.x)+abs(y-target.y);
        int rd = abs(d-target.dir);
        rd = std::min(rd,8-rd);
        return md*MOVE_COST + rd*ROTATE_COST;
    };

    g[start]=0;
    f[start]=heuristic(curX_,curY_,curDir_);
    open.push({f[start],start});

    while(!open.empty()){
        auto [fv,u] = open.top(); open.pop();
        if(u==goal) break;
        if(fv>f[u]) continue;

        int ux = (u/8) % cols;
        int uy = (u/8) / cols;
        int ud = u % 8;
        int base = g[u];

        // rotations
        for(auto &pr:ROTATIONS){
            int nd = (ud + pr.first + 8)%8;
            int v = idx(ux,uy,nd);
            int ng = base + ROTATE_COST;
            if(ng<g[v]){
                g[v]=ng;
                parent[v]=u;
                via[v]=pr.second;
                f[v]=ng + heuristic(ux,uy,nd);
                open.push({f[v],v});
            }
        }
        // forward
        int fx = ux + DX[ud];
        int fy = uy + DY[ud];
        if(isTraversable(fx,fy)){
            int v = idx(fx,fy,ud);
            int ng = base + MOVE_COST;
            if(ng<g[v]){
                g[v]=ng;
                parent[v]=u;
                via[v]=ActionRequest::MoveForward;
                f[v]=ng + heuristic(fx,fy,ud);
                open.push({f[v],v});
            }
        }
    }

    std::vector<ActionRequest> seq;
    if(parent[goal]<0) return seq;
    for(int v=goal; v!=start; v=parent[v])
        seq.push_back(via[v]);
    std::reverse(seq.begin(), seq.end());
    return seq;
}

std::vector<ActionRequest>
AggressiveTank::buildShootSequence(int walls) const
{
    std::vector<ActionRequest> s;
    int shots = walls*2+1;
    while(shots-->0) s.push_back(ActionRequest::Shoot);
    return s;
}

std::vector<ActionRequest> AggressiveTank::fallbackRoam() const {
    int rows = (int)lastInfo_.grid.size();
    int cols = rows? (int)lastInfo_.grid[0].size() : 0;
    int fx = (curX_ + DX[curDir_] + cols)%cols;
    int fy = (curY_ + DY[curDir_] + rows)%rows;
    char c = lastInfo_.grid[fy][fx];
    if(c!='@' && isTraversable(fx,fy))
        return { ActionRequest::MoveForward };
    return { ActionRequest::RotateRight45 };
}

void AggressiveTank::computePlan() {
    plan_.clear();
    auto shoots = findShootStates();
    std::cerr<<"DEBUG: findShootStates found "<<shoots.size()<<" candidates:\n";
    for(auto &ss:shoots){
      std::cerr<<"   dir="<<ss.st.dir
               <<" ds="<<ss.ds
               <<" walls="<<ss.walls<<"\n";
    }

    std::vector<ShootState> realShoots;
    int rows = (int)lastInfo_.grid.size();
    int cols = rows? (int)lastInfo_.grid[0].size() : 0;

    for(auto &ss:shoots){
      int tx = (curX_ + ss.ds*DX[ss.st.dir] + cols)%cols;
      int ty = (curY_ + ss.ds*DY[ss.st.dir] + rows)%rows;
      char cell = lastInfo_.grid[ty][tx];
      char e   = playerIndex_==1?'2':'1';
      std::cerr<<"DEBUG: checking ("<<tx<<","<<ty<<")='"
               <<cell<<"' vs '"<<e<<"'\n";
      if(cell==e) realShoots.push_back(ss);
    }

    if(realShoots.empty()){
      for(auto &a: fallbackRoam())
        plan_.push_back(a);
      return;
    }

    auto best = *std::min_element(
      realShoots.begin(), realShoots.end(),
      [&](auto &a,auto &b){
        int da = abs(a.st.x-curX_)+abs(a.st.y-curY_);
        int db = abs(b.st.x-curX_)+abs(b.st.y-curY_);
        return da<db;
      }
    );
    std::cerr<<"DEBUG: best shoot dir="<<best.st.dir
             <<" ds="<<best.ds<<" walls="<<best.walls<<"\n";

    // move & shoot
    for(auto &act: planPathTo(best.st))       plan_.push_back(act);
    for(auto &act: buildShootSequence(best.walls)) plan_.push_back(act);
}

bool AggressiveTank::lineOfSight(int sx,int sy,int dir,int &ds,int &wh) const {
    ds = wh = 0;
    int rows = (int)lastInfo_.grid.size();
    if(!rows) return false;
    int cols = (int)lastInfo_.grid[0].size();
    int x=sx, y=sy, steps=0, walls=0;
    while(true){
      x = (x + DX[dir] + cols)%cols;
      y = (y + DY[dir] + rows)%rows;
      if(++steps > rows*cols) return false;
      char c = lastInfo_.grid[y][x];
      if(c=='#'){ walls++; continue; }
      if(c=='@')   continue;
      if(c=='_'||c=='.'||c=='%'||c==' ') continue;
      // same-player tank blocks
      if((playerIndex_==1&&c=='1')||(playerIndex_==2&&c=='2'))
        return false;
      // enemy tank!
      if((playerIndex_==1&&c=='2')||(playerIndex_==2&&c=='1')){
        ds = steps;
        wh = walls;
        return true;
      }
      return false;
    }
}

bool AggressiveTank::isTraversable(int x,int y) const {
    int rows = (int)lastInfo_.grid.size();
    if(!rows) return false;
    int cols = (int)lastInfo_.grid[0].size();
    if(x<0||y<0||x>=cols||y>=rows) return false;
    char c = lastInfo_.grid[y][x];
    return (c=='_'||c=='.'||c=='%'||c==' ');
}
