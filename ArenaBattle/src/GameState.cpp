#include "GameState.h"
#include "Board.h"
#include "MyBattleInfo.h"
// #include "utils.h"
#include <iostream>
#include <sstream>


static const char* actionToString(common::ActionRequest a) {
    switch (a) {
      case common::ActionRequest::MoveForward:    return "MoveForward";
      case common::ActionRequest::MoveBackward:   return "MoveBackward";
      case common::ActionRequest::RotateLeft90:   return "RotateLeft90";
      case common::ActionRequest::RotateRight90:  return "RotateRight90";
      case common::ActionRequest::RotateLeft45:   return "RotateLeft45";
      case common::ActionRequest::RotateRight45:  return "RotateRight45";
      case common::ActionRequest::Shoot:          return "Shoot";
      case common::ActionRequest::GetBattleInfo:  return "GetBattleInfo";
      default:                                    return "DoNothing";
    }
}

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
                TankState ts{pidx,tidx,int(c),int(r),(pidx==1?6:2),true,num_shells_,0,0,false};
                
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

    // 1) Gather raw requests
    for (size_t k = 0; k < N; ++k) {
        auto& ts  = all_tanks_[k];
        auto& alg = *all_tank_algorithms_[k];
        if (!ts.alive) continue;

        ActionRequest req = alg.getAction();
        if (req == ActionRequest::GetBattleInfo) {
            // … your existing snapshot + updateTankWithBattleInfo …
            actions[k] = ActionRequest::GetBattleInfo;
        } else {
            actions[k] = req;
        }
    }
// ─── Just after “Gather raw requests” and before any rotations ─────────────────
// 1) Snapshot the original requests for logging
std::vector<ActionRequest> logActions = actions;

// 2) Backward‐delay logic (2 turns idle, 3rd turn executes)
for (size_t k = 0; k < N; ++k) {
    auto& ts   = all_tanks_[k];
    auto  orig = logActions[k];

    // (A) Mid‐delay from a previous MoveBackward?
    if (ts.backwardDelayCounter > 0) {
        --ts.backwardDelayCounter;
        if (ts.backwardDelayCounter == 0) {
            // 3rd turn → actually move backward
            ts.lastActionBackwardExecuted = true;
            actions[k] = ActionRequest::MoveBackward;
            ignored[k] = true;
        } else {
            // still in delay → only forward/info allowed
            if (orig == ActionRequest::MoveForward) {
                // cancel the delay
                ts.backwardDelayCounter       = 0;
                ts.lastActionBackwardExecuted = false;
                actions[k] = ActionRequest::DoNothing;
                ignored[k] = false;
            }
            else if (orig == ActionRequest::GetBattleInfo) {
                actions[k] = ActionRequest::GetBattleInfo;
                ignored[k] = false;
            }
            else {
                actions[k] = ActionRequest::DoNothing;
                ignored[k] = true;
            }
        }
        continue;
    }

    // (B) No pending delay: new MoveBackward request?
    if (orig == ActionRequest::MoveBackward) {
        // schedule exactly 2 idle turns then exec on the 3rd
        ts.backwardDelayCounter       = ts.lastActionBackwardExecuted ? 1 : 3;
        ts.lastActionBackwardExecuted = false;

        // do nothing this turn (exec will happen when counter→0)
        actions[k] = ActionRequest::DoNothing;
        ignored[k] = false;
        continue;
    }

    // (C) All other actions clear the “just did backward” flag
    ts.lastActionBackwardExecuted = false;
    // actions[k] remains orig; ignored[k] stays false
}


    // 2) Rotations
    applyTankRotations(actions);
    

    // 3) Mines
    handleTankMineCollisions();

    // 4) Cooldowns (unused)
    updateTankCooldowns();

    // 5) Backward legality check
    confirmBackwardMoves(ignored, actions);

    // 6) Shell movement & collisions
    updateShellsWithOverrunCheck();
    resolveShellCollisions();

    // 7) Shooting
    handleShooting(ignored, actions);

    // 8) Tank movement, collisions
    updateTankPositionsOnBoard(ignored, killed, actions);

    // 9) Cleanup shells & entities
    filterRemainingShells();
    cleanupDestroyedEntities();

    // 10) End‐of‐game
    checkGameEndConditions();

    // 11) Advance step & drop shoot cooldowns
    ++currentStep_;
    for (auto& ts : all_tanks_)
        if (ts.shootCooldown > 0) --ts.shootCooldown;


    // ─── Console print of each tank's decision and status ────────────────────────
    std::cout << "=== Decisions ===\n"<<std::endl;
    for (size_t k = 0; k < N; ++k) {
        const char* actName = actionToString(logActions[k]);
        bool wasIgnored     = ignored[k]
        && logActions[k] != common::ActionRequest::GetBattleInfo;
        std::cout << "  Tank[" << k << "]: "
        << actName
        << (wasIgnored ? " (ignored)" : " (accepted)")
        << "\n";
    }
    std::cout << std::endl;
    std::cout << "=== Board State: ===\n" << std::endl;
    
    // ─── Logging: use the ORIGINAL requests ────────────────────────────────────
    std::ostringstream oss;
    for (size_t k = 0; k < N; ++k) {
        const auto act = logActions[k];
        const char* name = "DoNothing";
        switch (act) {
          case ActionRequest::MoveForward:    name = "MoveForward";   break;
          case ActionRequest::MoveBackward:   name = "MoveBackward";  break;
          case ActionRequest::RotateLeft90:   name = "RotateLeft90";  break;
          case ActionRequest::RotateRight90:  name = "RotateRight90"; break;
          case ActionRequest::RotateLeft45:   name = "RotateLeft45";  break;
          case ActionRequest::RotateRight45:  name = "RotateRight45"; break;
          case ActionRequest::Shoot:          name = "Shoot";         break;
          case ActionRequest::GetBattleInfo:  name = "GetBattleInfo"; break;
          default:                                                    break;
        }

        if (all_tanks_[k].alive) {
            oss << name;
            if (ignored[k] && act != ActionRequest::GetBattleInfo)
            {
                oss << " (ignored)";
            }
        } else {
            // tank died this turn
            if (act == ActionRequest::DoNothing)
                oss << "killed";
            else
                oss << name << " (killed)";
        }
        if (k + 1 < N) oss << ", ";
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
                std::cout << (cell.hasShellOverlay? '*' : '_');
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

//------------------------------------------------------------------------------
// 5) Confirm backward–move legality (now with wrapping)
void GameState::confirmBackwardMoves(std::vector<bool>& ignored,
                                     const std::vector<ActionRequest>& A)
{
    for (size_t k = 0; k < all_tanks_.size(); ++k) {
        if (!all_tanks_[k].alive || A[k] != ActionRequest::MoveBackward)
            continue;

        // compute backward direction
        int back = (all_tanks_[k].direction + 4) & 7;
        int dx = 0, dy = 0;
        switch (back) {
        case 0: dy = -1; break;
        case 1: dx = +1; dy = -1; break;
        case 2: dx = +1; break;
        case 3: dx = +1; dy = +1; break;
        case 4: dy = +1; break;
        case 5: dx = -1; dy = +1; break;
        case 6: dx = -1; break;
        case 7: dx = -1; dy = -1; break;
        }

        int nx = all_tanks_[k].x + dx;
        int ny = all_tanks_[k].y + dy;
        // wrap around
        board_.wrapCoords(nx, ny);
        // illegal if there's a wall after wrapping
        if (board_.getCell(nx, ny).content == CellContent::WALL) {
            ignored[k] = true;
        }
    }
}

//------------------------------------------------------------------------------
// (6) updateTankPositionsOnBoard: move forward/backward, no wrap,
//     plus handle head‐on tank collisions and multi‐tank collisions.
//------------------------------------------------------------------------------
void GameState::updateTankPositionsOnBoard(std::vector<bool>& ignored,
                                           std::vector<bool>& killedThisTurn,
                                           const std::vector<common::ActionRequest>& actions)
{
     board_.clearTankMarks();

    const size_t N = all_tanks_.size();
    std::vector<std::pair<int,int>> oldPos(N), newPos(N);

    // 1) compute oldPos & newPos (with wrapping)
    for (size_t k = 0; k < N; ++k) {
        oldPos[k] = { all_tanks_[k].x, all_tanks_[k].y };

        if (!all_tanks_[k].alive
         || ignored[k]
         || (actions[k] != ActionRequest::MoveForward
          && actions[k] != ActionRequest::MoveBackward))
        {
            newPos[k] = oldPos[k];
            continue;
        }

        // figure out dx,dy
        int dir = all_tanks_[k].direction;
        if (actions[k] == ActionRequest::MoveBackward)
            dir = (dir + 4) & 7;

        int dx = 0, dy = 0;
        switch (dir) {
        case 0: dy = -1; break;
        case 1: dx = +1; dy = -1; break;
        case 2: dx = +1; break;
        case 3: dx = +1; dy = +1; break;
        case 4: dy = +1; break;
        case 5: dx = -1; dy = +1; break;
        case 6: dx = -1; break;
        case 7: dx = -1; dy = -1; break;
        }

        int nx = all_tanks_[k].x + dx;
        int ny = all_tanks_[k].y + dy;
        // wrap around the board edges
        board_.wrapCoords(nx, ny);

        // if after wrapping there's a wall, treat as ignored
        if (board_.getCell(nx, ny).content == CellContent::WALL) {
            newPos[k] = oldPos[k];
            ignored[k] = true;
        } else {
            newPos[k] = { nx, ny };
        }
    }

    // 2a) Head-on swaps: two tanks exchanging places → both die
    for (std::size_t i = 0; i < N; ++i) {
      for (std::size_t j = i+1; j < N; ++j) {
        if (!all_tanks_[i].alive || !all_tanks_[j].alive) continue;
        if (killedThisTurn[i] || killedThisTurn[j])        continue;
        if (newPos[i] == oldPos[j] && newPos[j] == oldPos[i]) {
          killedThisTurn[i] = killedThisTurn[j] = true;
          all_tanks_[i].alive = all_tanks_[j].alive = false;
          // clear both old positions
          board_.setCell(oldPos[i].first, oldPos[i].second, CellContent::EMPTY);
          board_.setCell(oldPos[j].first, oldPos[j].second, CellContent::EMPTY);
        }
      }
    }

    // 2b) Moving-into-stationary: a mover steps onto someone who stayed put → both die
    for (std::size_t k = 0; k < N; ++k) {
      if (!all_tanks_[k].alive                   ) continue;  // dead already
      if (killedThisTurn[k]                      ) continue;  // marked in 2a
      if (newPos[k] == oldPos[k]) continue;                 // didn’t move
      for (std::size_t j = 0; j < N; ++j) {
        if (j == k)                                             continue;
        if (!all_tanks_[j].alive                              ) continue;  // dead
        if (killedThisTurn[j]                                 ) continue;  // marked
        if (newPos[j] != oldPos[j]) continue;                   // j must be stationary
        if (newPos[k] == oldPos[j]) {
          // k moved into j’s square
          killedThisTurn[k] = killedThisTurn[j] = true;
          all_tanks_[k].alive = all_tanks_[j].alive = false;
          board_.setCell(oldPos[k].first, oldPos[k].second, CellContent::EMPTY);
          board_.setCell(oldPos[j].first, oldPos[j].second, CellContent::EMPTY);
        }
      }
    }

    // 2c) Multi-tank collisions at same destination: any cell with ≥2 movers → all die
    std::map<std::pair<int,int>, std::vector<std::size_t>> destMap;
    for (std::size_t k = 0; k < N; ++k) {
      if (!all_tanks_[k].alive 
       || killedThisTurn[k] 
       || newPos[k] == oldPos[k]) continue;
      destMap[newPos[k]].push_back(k);
    }
    for (auto const& [pos, vec] : destMap) {
      if (vec.size() > 1) {
        for (auto k : vec) {
          if (!all_tanks_[k].alive || killedThisTurn[k]) continue;
          killedThisTurn[k]   = true;
          all_tanks_[k].alive = false;
          // clear their old position
          board_.setCell(oldPos[k].first, oldPos[k].second, CellContent::EMPTY);
        }
      }
    }


    // 3) Now apply every non‐colliding move
    for (std::size_t k = 0; k < N; ++k) {
        if (!all_tanks_[k].alive)
            continue;

        auto [ox, oy] = oldPos[k];
        auto [nx, ny] = newPos[k];

        // stayed in place?
        if (nx == ox && ny == oy) {
            board_.setCell(ox, oy,
                all_tanks_[k].player_index == 1
                  ? CellContent::TANK1
                  : CellContent::TANK2
            );
            continue;
        }

        // illegal: wall
        if (board_.getCell(nx, ny).content == CellContent::WALL) {
            ignored[k] = true;
            board_.setCell(ox, oy,
                all_tanks_[k].player_index == 1
                  ? CellContent::TANK1
                  : CellContent::TANK2
            );
            continue;
        }

        // --- NEW: mutual shell‐tank destruction ---
        {
            bool collidedWithShell = false;
            for (size_t s = 0; s < shells_.size(); ++s) {
                if (shells_[s].x == nx && shells_[s].y == ny) {
                    // kill tank
                    all_tanks_[k].alive = false;
                    killedThisTurn[k]   = true;
                    // clear its old cell
                    board_.setCell(ox, oy, CellContent::EMPTY);
                    board_.setCell(nx, ny, CellContent::EMPTY);
                    // remove that shell
                    shells_.erase(shells_.begin() + s);
                    collidedWithShell = true;
                    break;
                }
            }
            if (collidedWithShell)
                continue;  // tank is dead, skip the rest
        }

        // mine → both die
        if (board_.getCell(nx, ny).content == CellContent::MINE) {
            killedThisTurn[k]   = true;
            all_tanks_[k].alive = false;
            board_.setCell(ox, oy, CellContent::EMPTY);
            board_.setCell(nx, ny, CellContent::EMPTY);
            continue;
        }

        // normal move
        board_.setCell(ox, oy, CellContent::EMPTY);
        all_tanks_[k].x = nx;
        all_tanks_[k].y = ny;
        board_.setCell(nx, ny,
            all_tanks_[k].player_index == 1
              ? CellContent::TANK1
              : CellContent::TANK2
        );
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

    for (size_t k = 0; k < all_tanks_.size(); ++k) {
        auto& ts = all_tanks_[k];
        if (!ts.alive || A[k] != ActionRequest::Shoot) continue;

        // 1) still cooling down?
        if (ts.shootCooldown > 0) {
            ignored[k] = true;
            continue;
        }
        // 2) out of ammo?
        if (ts.shells_left == 0) {
            ignored[k] = true;
            continue;
        }
        // 3) fire!
        ts.shells_left--;
        ts.shootCooldown = 4;    // set 4‐turn cooldown
        spawn(ts);
    }
}
//------------------------------------------------------------------------------
// (8) updateShellsWithOverrunCheck:
//     move each shell in two unit-steps, checking for tank/wall at each step
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// (6.5) updateShellsWithOverrunCheck:
//     move each shell in two unit-steps, checking for wall/tank collisions
//     and also if two shells cross through each other.
void GameState::updateShellsWithOverrunCheck() {
    toRemove_.clear();
    positionMap_.clear();
    board_.clearShellMarks();

    const size_t S = shells_.size();
    // 1) snapshot old positions and deltas
    std::vector<std::pair<int,int>> oldPos(S);
    std::vector<std::pair<int,int>> delta(S);
    for (size_t i = 0; i < S; ++i) {
        oldPos[i] = { shells_[i].x, shells_[i].y };
        int dx = 0, dy = 0;
        switch (shells_[i].dir) {
          case 0:  dy = -1; break;
          case 1:  dx = +1; dy = -1; break;
          case 2:  dx = +1; break;
          case 3:  dx = +1; dy = +1; break;
          case 4:  dy = +1; break;
          case 5:  dx = -1; dy = +1; break;
          case 6:  dx = -1; break;
          case 7:  dx = -1; dy = -1; break;
        }
        delta[i] = {dx, dy};
    }

    // 2) perform two sub-steps simultaneously
    for (int step = 0; step < 2; ++step) {
        for (size_t i = 0; i < shells_.size(); ++i) {
            if (toRemove_.count(i)) continue;  // already dying

            // compute this shell's next position
            int nx = shells_[i].x + delta[i].first;
            int ny = shells_[i].y + delta[i].second;
            board_.wrapCoords(nx, ny);

            // 2a) crossing-paths check
            for (size_t j = 0; j < shells_.size(); ++j) {
                if (i == j || toRemove_.count(j)) continue;
                // j's old and would-be new pos
                auto [oxj, oyj] = oldPos[j];
                int nxj = oxj + delta[j].first;
                int nyj = oyj + delta[j].second;
                board_.wrapCoords(nxj, nyj);
                // if i’s new == j’s old AND j’s new == i’s old → cross
                if (nx == oxj && ny == oyj
                 && nxj == oldPos[i].first
                 && nyj == oldPos[i].second)
                {
                    toRemove_.insert(i);
                    toRemove_.insert(j);
                    break;
                }
            }
            if (toRemove_.count(i)) continue;

            // advance the shell
            shells_[i].x = nx;
            shells_[i].y = ny;

            // 2b) tank/wall mid-step collision
            if (handleShellMidStepCollision(nx, ny)) {
                toRemove_.insert(i);
                continue;
            }

            // 2c) record for same-cell collisions
            positionMap_[{nx, ny}].push_back(i);
        }
    }
}


void GameState::resolveShellCollisions() {
    // if two or more shells occupy the same cell, they all die
    for (auto const& entry : positionMap_) {
        const auto& idxs = entry.second;
        if (idxs.size() > 1) {
            for (auto idx : idxs) {
                toRemove_.insert(idx);
            }
        }
    }
}

void GameState::filterRemainingShells() {
    std::vector<Shell> remaining;
    remaining.reserve(shells_.size());
    for (std::size_t i = 0; i < shells_.size(); ++i) {
        // survivors are those not in toRemove_
        if (toRemove_.count(i) == 0) {
            // mark overlay
            auto& cell = board_.getCell(shells_[i].x, shells_[i].y);
            cell.hasShellOverlay = true;
            remaining.push_back(shells_[i]);
        }
    }
    shells_.swap(remaining);
}

//------------------------------------------------------------------------------
// (I) handleShellMidStepCollision: wall/tank logic at (x,y)
//------------------------------------------------------------------------------
bool GameState::handleShellMidStepCollision(int x, int y) {
    Cell& cell = board_.getCell(x, y);

    // 1) Wall?
    if (cell.content == CellContent::WALL) {
        cell.wallHits++;
        if (cell.wallHits >= 2) {
            cell.content = CellContent::EMPTY;
        }
        return true;
    }

    // 2) Tank?
    if (cell.content == CellContent::TANK1 || cell.content == CellContent::TANK2) {
        // find and kill the matching TankState
        int pid = (cell.content == CellContent::TANK1 ? 1 : 2);
        for (auto& ts : all_tanks_) {
            if (ts.alive && ts.player_index == pid && ts.x == x && ts.y == y) {
                ts.alive = false;
                break;
            }
        }
        cell.content = CellContent::EMPTY;
        return true;
    }

    // 3) Mine or empty: shells pass through mines, are only removed on tanks/walls
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
