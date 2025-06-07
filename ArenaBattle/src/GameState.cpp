#include "GameState.h"
#include "Board.h"
#include "MyBattleInfo.h"

#include <iostream>
#include <sstream>

using namespace arena;
using namespace common;

//------------------------------------------------------------------------------
GameState::GameState(std::unique_ptr<common::PlayerFactory> pFac,
                     std::unique_ptr<common::TankAlgorithmFactory> tFac)
  : player_factory_(std::move(pFac)),
    tank_factory_(std::move(tFac))
{}

GameState::~GameState() = default;

//------------------------------------------------------------------------------
void GameState::initialize(const Board& board,
                           std::size_t maxSteps,
                           std::size_t numShells)
{
    board_      = board;
    rows_       = board.getRows();
    cols_       = board.getCols();
    maxSteps_   = maxSteps;
    num_shells_ = numShells;

    all_tanks_.clear();
    tankIdMap_.assign(3, std::vector<std::size_t>(rows_*cols_, SIZE_MAX));
    nextTankIndex_[1] = nextTankIndex_[2] = 0;

    for (std::size_t r = 0; r < rows_; ++r) {
        for (std::size_t c = 0; c < cols_; ++c) {
            const Cell& cell = board_.getCell(int(c), int(r));
            if (cell.content == CellContent::TANK1 ||
                cell.content == CellContent::TANK2)
            {
                int pidx = (cell.content==CellContent::TANK1?1:2);
                int tidx = nextTankIndex_[pidx]++;
                TankState ts{pidx,tidx,int(c),int(r),(pidx==1?6:2),true,num_shells_,false};
                all_tanks_.push_back(ts);
                tankIdMap_[pidx][tidx] = all_tanks_.size()-1;
            }
        }
    }

    player1_ = player_factory_->create(1, rows_, cols_, maxSteps_, num_shells_);
    player2_ = player_factory_->create(2, rows_, cols_, maxSteps_, num_shells_);

    all_tank_algorithms_.clear();
    for (auto& ts : all_tanks_) {
        all_tank_algorithms_.push_back(
            tank_factory_->create(ts.player_index, ts.tank_index)
        );
    }

    shells_.clear();
    toRemove_.clear();
    positionMap_.clear();

    currentStep_ = 0;
    gameOver_    = false;
    resultStr_.clear();
}

//------------------------------------------------------------------------------
std::string GameState::advanceOneTurn() {
    if (gameOver_) return "";

    const size_t N = all_tanks_.size();
    std::vector<ActionRequest> actions(N, ActionRequest::DoNothing);
    std::vector<bool> ignored(N,false), killed(N,false);

    // 1) Gather actions
    for (size_t k = 0; k < N; ++k) {
        auto& ts  = all_tanks_[k];
        auto& alg = *all_tank_algorithms_[k];
        if (!ts.alive) continue;

        ActionRequest req = alg.getAction();
        if (req == ActionRequest::GetBattleInfo) {
            // build snapshot grid
            std::vector<std::vector<char>> grid(rows_, std::vector<char>(cols_, ' '));
            for (size_t y=0; y<rows_; ++y)
                for (size_t x=0; x<cols_; ++x) {
                    const auto& cell = board_.getCell(int(x),int(y));
                    grid[y][x] = (cell.content==CellContent::WALL?'#':
                                  cell.content==CellContent::MINE?'@':
                                  cell.content==CellContent::TANK1?'1':
                                  cell.content==CellContent::TANK2?'2':' ');
                }
            grid[ts.y][ts.x] = '%';
            MySatelliteView sv(grid,int(rows_),int(cols_), ts.x, ts.y);
            if (ts.player_index==1)
                player1_->updateTankWithBattleInfo(alg, sv);
            else
                player2_->updateTankWithBattleInfo(alg, sv);
            actions[k] = ActionRequest::GetBattleInfo;
        } else {
            actions[k] = req;
        }
    }

    // 2) Rotations
    applyTankRotations(actions);
    // 3) Mines
    handleTankMineCollisions();
    // 4) Cooldowns
    updateTankCooldowns();
    // 5) Backward legality
    confirmBackwardMoves(ignored, actions);
    // 6) Move tanks
    updateTankPositionsOnBoard(ignored, killed, actions);
    // 7) Spawn shells
    handleShooting(ignored, actions);
    // 8) Move shells
    updateShellsWithOverrunCheck();
    // 9) Resolve collisions
    resolveShellCollisions();
    // 10) Cleanup
    cleanupDestroyedEntities();
    // 11) End game?
    checkGameEndConditions();
    // 12) Next turn
    ++currentStep_;

    // Build log
    std::ostringstream oss;
    for (size_t k=0; k<N; ++k) {
        if (!all_tanks_[k].alive) {
            oss << "killed";
        } else {
            const auto act = actions[k];
            const char* name = "DoNothing";
            switch(act) {
            case ActionRequest::MoveForward:   name="MoveForward"; break;
            case ActionRequest::MoveBackward:  name="MoveBackward";break;
            case ActionRequest::RotateLeft90:  name="RotateLeft90"; break;
            case ActionRequest::RotateRight90: name="RotateRight90";break;
            case ActionRequest::RotateLeft45:  name="RotateLeft45"; break;
            case ActionRequest::RotateRight45: name="RotateRight45";break;
            case ActionRequest::Shoot:         name="Shoot";        break;
            case ActionRequest::GetBattleInfo: name="GetBattleInfo";break;
            default: break;
            }
            oss << name;
            if (ignored[k] && (act==ActionRequest::MoveForward ||
                              act==ActionRequest::MoveBackward ||
                              act==ActionRequest::Shoot))
                oss << " (ignored)";
            if (killed[k])
                oss << " (killed)";
        }
        if (k+1<N) oss << ", ";
    }
    return oss.str();
}

//------------------------------------------------------------------------------
bool GameState::isGameOver() const { return gameOver_; }
std::string GameState::getResultString() const { return resultStr_; }

//------------------------------------------------------------------------------
void GameState::printBoard() const {
    auto gridCopy = board_.getGrid();
    for (auto const& sh : shells_)
        gridCopy[sh.y][sh.x].hasShellOverlay = true;
    for (auto const& ts : all_tanks_)
        if (ts.alive)
            gridCopy[ts.y][ts.x].content = 
                ts.player_index==1?CellContent::TANK1:CellContent::TANK2;

    for (size_t r=0; r<rows_; ++r) {
        for (size_t c=0; c<cols_; ++c) {
            const auto& cell = gridCopy[r][c];
            switch(cell.content) {
            case CellContent::WALL:  std::cout<<'#'; break;
            case CellContent::MINE:  std::cout<<'@'; break;
            case CellContent::EMPTY:
                std::cout << (cell.hasShellOverlay? '*' : ' ');
                break;
            case CellContent::TANK1:
            case CellContent::TANK2: {
                int pid = (cell.content==CellContent::TANK1?1:2);
                int dir=0;
                for (auto const& ts: all_tanks_) {
                    if (ts.alive && ts.player_index==pid &&
                        ts.x==int(c)&&ts.y==int(r)) { dir=ts.direction; break; }
                }
                const char* arr = directionToArrow(dir);
                std::cout << (pid==1? "\033[31m": "\033[34m")
                          << arr << "\033[0m";
            } break;
            }
        }
        std::cout<<"\n";
    }
    std::cout<<std::endl;
}

//------------------------------------------------------------------------------
const char* GameState::directionToArrow(int dir) {
    static const char* arr[8] = {"↑","↗","→","↘","↓","↙","←","↖"};
    return arr[dir&7];
}

//------------------------------------------------------------------------------
void GameState::applyTankRotations(const std::vector<ActionRequest>& A) {
    for (size_t k=0; k<all_tanks_.size(); ++k) {
        if (!all_tanks_[k].alive) continue;
        int& d = all_tanks_[k].direction;
        switch(A[k]) {
        case ActionRequest::RotateLeft90:  d=(d+6)&7; break;
        case ActionRequest::RotateRight90: d=(d+2)&7; break;
        case ActionRequest::RotateLeft45:  d=(d+7)&7; break;
        case ActionRequest::RotateRight45: d=(d+1)&7; break;
        default: break;
        }
    }
}

void GameState::handleTankMineCollisions() {
    for (auto& ts: all_tanks_) {
        if (!ts.alive) continue;
        auto& cell = board_.getCell(ts.x, ts.y);
        if (cell.content==CellContent::MINE) {
            ts.alive = false;
            cell.content = CellContent::EMPTY;
        }
    }
}

void GameState::updateTankCooldowns() {
    // unused
}

void GameState::confirmBackwardMoves(std::vector<bool>& ignored,
                                     const std::vector<ActionRequest>& A)
{
    for (size_t k=0; k<all_tanks_.size(); ++k) {
        if (!all_tanks_[k].alive || A[k]!=ActionRequest::MoveBackward)
            continue;
        int back = (all_tanks_[k].direction+4)&7;
        int dx=0,dy=0;
        switch(back) {
        case 0: dy=-1; break; case 1: dx=1;dy=-1; break;
        case 2: dx=1; break;  case 3: dx=1;dy=1; break;
        case 4: dy=1; break;  case 5: dx=-1;dy=1; break;
        case 6: dx=-1; break; case 7: dx=-1;dy=-1; break;
        }
        int nx=all_tanks_[k].x+dx, ny=all_tanks_[k].y+dy;
        board_.wrapCoords(nx,ny);
        if (board_.getCell(nx,ny).content==CellContent::WALL)
            ignored[k]=true;
    }
}

void GameState::updateTankPositionsOnBoard(std::vector<bool>& ignored,
                                           std::vector<bool>& killed,
                                           const std::vector<ActionRequest>& A)
{
    board_.clearTankMarks();
    for (size_t k=0; k<all_tanks_.size(); ++k) {
        auto& ts = all_tanks_[k];
        if (!ts.alive) continue;
        if ((A[k]!=ActionRequest::MoveForward && A[k]!=ActionRequest::MoveBackward) ||
            ignored[k]) continue;
        int dir = ts.direction;
        if (A[k]==ActionRequest::MoveBackward) dir=(dir+4)&7;
        int dx=0,dy=0;
        switch(dir) {
        case 0: dy=-1; break; case 1: dx=1;dy=-1; break;
        case 2: dx=1; break;  case 3: dx=1;dy=1; break;
        case 4: dy=1; break;  case 5: dx=-1;dy=1; break;
        case 6: dx=-1; break; case 7: dx=-1;dy=-1; break;
        }
        int nx=ts.x+dx, ny=ts.y+dy;
        board_.wrapCoords(nx,ny);
        if (board_.getCell(nx,ny).content==CellContent::WALL) {
            ignored[k]=true;
            continue;
        }
        if (board_.getCell(nx,ny).content==CellContent::MINE) {
            killed[k]=true;
            ts.alive=false;
            board_.setCell(ts.x,ts.y,CellContent::EMPTY);
            board_.setCell(nx,ny,CellContent::EMPTY);
            continue;
        }
        board_.setCell(ts.x,ts.y,CellContent::EMPTY);
        ts.x = nx; ts.y = ny;
        board_.setCell(nx,ny,
            ts.player_index==1?CellContent::TANK1:CellContent::TANK2);
    }
}

void GameState::handleShooting(std::vector<bool>& ignored,
                               const std::vector<ActionRequest>& A)
{
    auto spawn = [&](TankState& ts){
        int dx=0,dy=0;
        switch(ts.direction) {
        case 0: dy=-1; break; case 1: dx=1;dy=-1; break;
        case 2: dx=1; break;  case 3: dx=1;dy=1; break;
        case 4: dy=1; break;  case 5: dx=-1;dy=1; break;
        case 6: dx=-1; break; case 7: dx=-1;dy=-1; break;
        }
        int sx=(ts.x+dx+board_.getWidth())%board_.getWidth();
        int sy=(ts.y+dy+board_.getHeight())%board_.getHeight();
        if (!handleShellMidStepCollision(sx,sy))
            shells_.push_back({sx,sy,ts.direction});
    };

    for (size_t k=0; k<all_tanks_.size(); ++k) {
        if (!all_tanks_[k].alive || A[k]!=ActionRequest::Shoot) continue;
        if (all_tanks_[k].shells_left==0) {
            ignored[k]=true;
            continue;
        }
        all_tanks_[k].shells_left--;
        spawn(all_tanks_[k]);
    }
}

void GameState::updateShellsWithOverrunCheck() {
    toRemove_.clear();
    positionMap_.clear();
    board_.clearShellMarks();

    for (size_t i=0; i<shells_.size(); ++i) {
        int dx=0,dy=0;
        switch(shells_[i].dir) {
        case 0: dy=-1; break; case 1: dx=1;dy=-1; break;
        case 2: dx=1; break;  case 3: dx=1;dy=1; break;
        case 4: dy=1; break;  case 5: dx=-1;dy=1; break;
        case 6: dx=-1; break; case 7: dx=-1;dy=-1; break;
        }
        for (int step=0; step<2; ++step) {
            int nx=shells_[i].x+dx, ny=shells_[i].y+dy;
            board_.wrapCoords(nx,ny);
            if (handleShellMidStepCollision(nx,ny)) {
                toRemove_.insert(i);
                break;
            }
            shells_[i].x=nx; shells_[i].y=ny;
            positionMap_[{nx,ny}].push_back(i);
        }
    }
}

void GameState::resolveShellCollisions() {
    for (auto const& kv : positionMap_) {
        if (kv.second.size()>1)
            for (auto idx : kv.second)
                toRemove_.insert(idx);
    }
    std::vector<Shell> survivors;
    for (size_t i=0; i<shells_.size(); ++i) {
        if (!toRemove_.count(i)) {
            auto& cell = board_.getCell(shells_[i].x, shells_[i].y);
            cell.hasShellOverlay = true;
            survivors.push_back(shells_[i]);
        }
    }
    shells_.swap(survivors);
}

bool GameState::handleShellMidStepCollision(int x, int y) {
    auto& cell = board_.getCell(x,y);
    if (cell.content==CellContent::WALL) {
        if (++cell.wallHits >=2) cell.content=CellContent::EMPTY;
        return true;
    }
    if (cell.content==CellContent::TANK1 || cell.content==CellContent::TANK2) {
        for (auto& ts: all_tanks_) {
            if (ts.alive && ts.x==x && ts.y==y &&
               ((cell.content==CellContent::TANK1&&ts.player_index==1)||
                (cell.content==CellContent::TANK2&&ts.player_index==2))) {
                ts.alive=false;
                break;
            }
        }
        cell.content=CellContent::EMPTY;
        return true;
    }
    return false;
}

void GameState::cleanupDestroyedEntities() {
    // nothing
}

void GameState::checkGameEndConditions() {
    int a1=0,a2=0;
    for (auto const& ts: all_tanks_) {
        if (ts.alive) (ts.player_index==1?++a1:++a2);
    }
    if (a1==0 && a2==0) {
        gameOver_=true; resultStr_="Tie, both players have zero tanks";
    }
    else if (a1==0) {
        gameOver_=true; resultStr_="Player 2 won with "+std::to_string(a2)+" tanks still alive";
    }
    else if (a2==0) {
        gameOver_=true; resultStr_="Player 1 won with "+std::to_string(a1)+" tanks still alive";
    }
    else if (currentStep_+1>=maxSteps_) {
        gameOver_=true;
        resultStr_="Tie, reached max steps="+std::to_string(maxSteps_)+
                   ", player1 has "+std::to_string(a1)+
                   ", player2 has "+std::to_string(a2);
    }
}
