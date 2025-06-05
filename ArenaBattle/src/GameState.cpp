#include "GameState.h"
#include <iostream>
#include <sstream>

using namespace arena;
using namespace common;

//------------------------------------------------------------------------------
// Constructor / Destructor
//------------------------------------------------------------------------------
GameState::GameState(std::unique_ptr<common::PlayerFactory> pFac,
                     std::unique_ptr<common::TankAlgorithmFactory> tFac)
    : player_factory_(std::move(pFac)),
      tank_factory_(std::move(tFac))
{
    // Nothing else here; must call initialize() next.
}

GameState::~GameState() = default;

//------------------------------------------------------------------------------
// initialize:
//   1) copy board_, rows_, cols_, maxSteps_, num_shells_
//   2) scan the board for '1'/'2' cells → build TankState + tankIdMap_
//   3) create two Player instances via player_factory_->create(...)
//   4) create one TankAlgorithm instance per TankState via tank_factory_->create(...)
//   5) set currentStep_ = 0, gameOver_ = false
//------------------------------------------------------------------------------
void GameState::initialize(const Board& board, std::size_t maxSteps, std::size_t numShells) {
    // Copy board
    board_ = board;
    rows_ = board.getRows();
    cols_ = board.getCols();

    // Store maxSteps and shells
    maxSteps_ = maxSteps;
    num_shells_ = numShells;

    // 1) Scan for tanks on the board:
    nextTankIndex_[1] = nextTankIndex_[2] = 0;
    all_tanks_.clear();
    tankIdMap_.assign(3, std::vector<std::size_t>(rows_ * cols_, SIZE_MAX));

    for (std::size_t r = 0; r < rows_; ++r) {
        for (std::size_t c = 0; c < cols_; ++c) {
            char ch = board_.getCell(r, c);
            if (ch == '1' || ch == '2') {
                int pidx = (ch == '1' ? 1 : 2);
                int tidx = nextTankIndex_[pidx]++;
                TankState ts;
                ts.player_index = pidx;
                ts.tank_index = tidx;
                ts.x = r;
                ts.y = c;
                ts.direction = (pidx == 1 ? 6 : 2); // Player1 faces Left=6, Player2 faces Right=2
                ts.alive = true;
                ts.shells_left = num_shells_;
                ts.wantsBackward_ = false;
                all_tanks_.push_back(ts);

                std::size_t globalIdx = all_tanks_.size() - 1;
                if (static_cast<std::size_t>(tidx) >= tankIdMap_[pidx].size()) {
                    tankIdMap_[pidx].resize(tidx + 1, SIZE_MAX);
                }
                tankIdMap_[pidx][tidx] = globalIdx;
            }
        }
    }

    // 2) Create two Player instances (pass rows_, cols_, maxSteps_, num_shells_)
    player1_ = player_factory_->create(1, rows_, cols_, maxSteps_, num_shells_);
    player2_ = player_factory_->create(2, rows_, cols_, maxSteps_, num_shells_);

    // 3) Create one TankAlgorithm instance per TankState
    all_tank_algorithms_.clear();
    all_tank_algorithms_.reserve(all_tanks_.size());
    for (std::size_t k = 0; k < all_tanks_.size(); ++k) {
        int pidx = all_tanks_[k].player_index;
        int tidx = all_tanks_[k].tank_index;
        all_tank_algorithms_.push_back(
            tank_factory_->create(pidx, tidx)
        );
    }

    // 4) Clear projectiles, reset turn counters
    projectiles_.clear();
    currentStep_ = 0;
    gameOver_ = false;
    resultStr_.clear();
}

//------------------------------------------------------------------------------
// advanceOneTurn:
//   1) For each alive tank k, call tankAlg->getAction().  If it returns GetBattleInfo,
//      build a MySatelliteView (copy of board_.getGrid() with '%' at that tank’s (x,y)),
//      then call the correct Player->updateTankWithBattleInfo(...) so the AI can see the grid.
//      Then call getAction() again to get the “real” move this turn.  Store in actions[k].
//   2) applyTankRotations(actions)
//   3) handleTankMineCollisions()
//   4) updateTankCooldowns()
//   5) confirmBackwardMoves(ignored, actions)  (marks “ignored” for illegal backward moves)
//   6) updateTankPositionsOnBoard(ignored, killedThisTurn, actions)
//   7) advanceProjectiles()
//   8) resolveProjectileCollisions(killedThisTurn)
//   9) handleShooting(actions, killedThisTurn)
//  10) cleanupDestroyedEntities()
//  11) checkGameEndConditions()
//  12) currentStep_++
//
//  Finally, produce a single comma‐separated string describing each tank’s action, e.g.
//      "RotateRight90, MoveForward (ignored), Shoot (killed), DoNothing"
//------------------------------------------------------------------------------
std::string GameState::advanceOneTurn() {
    // If already gameOver_, return an empty string
    if (gameOver_) return "";

    std::size_t N = all_tanks_.size();
    std::vector<common::ActionRequest> actions(N, common::ActionRequest::DoNothing);
    std::vector<bool> ignored(N, false);
    std::vector<bool> killedThisTurn(N, false);

    // --- (1) Collect each tank’s requested action ---
    for (std::size_t k = 0; k < N; ++k) {
        if (!all_tanks_[k].alive) continue;
        auto& alg = all_tank_algorithms_[k];

        // First call:
        common::ActionRequest req = alg->getAction();
        if (req == common::ActionRequest::GetBattleInfo) {
            // Build a MySatelliteView for THIS tank:
            MySatelliteView satView(
                board_.getGrid(),    // full grid
                rows_, cols_,        // dimensions
                static_cast<int>(all_tanks_[k].x),
                static_cast<int>(all_tanks_[k].y)
            );

            // Send to the correct Player:
            if (all_tanks_[k].player_index == 1) {
                player1_->updateTankWithBattleInfo(*alg, satView);
            } else {
                player2_->updateTankWithBattleInfo(*alg, satView);
            }
            // Now get the “real” action:
            req = alg->getAction();
        }
        actions[k] = req;
    }

    // --- (2) Rotations ---
    applyTankRotations(actions);

    // --- (3) Mines collision ---
    handleTankMineCollisions();

    // --- (4) Cooldowns (if you implement any) ---
    updateTankCooldowns();

    // --- (5) Confirm backward‐move legality ---
    confirmBackwardMoves(ignored, actions);

    // --- (6) Update tank positions on the board (forward/backward) ---
    updateTankPositionsOnBoard(ignored, killedThisTurn, actions);

    // --- (7) Advance all projectiles one step ---
    advanceProjectiles();

    // --- (8) Resolve projectile collisions (wall, tank) ---
    resolveProjectileCollisions(killedThisTurn);

    // --- (9) Handle shooting (spawn new projectiles) ---
    handleShooting(actions, killedThisTurn);

    // --- (10) Cleanup destroyed entities ---
    cleanupDestroyedEntities();

    // --- (11) Check end‐of‐game conditions ---
    checkGameEndConditions();

    // --- (12) Increment step counter ---
    ++currentStep_;

    // --- Build and return the comma‐separated “turn string” ---
    std::ostringstream oss;
    for (std::size_t k = 0; k < N; ++k) {
        if (!all_tanks_[k].alive) {
            // This tank is dead now
            oss << "killed";
        } else {
            // Still alive → convert actions[k] to a string
            std::string s;
            switch (actions[k]) {
                case common::ActionRequest::MoveForward:   s = "MoveForward";   break;
                case common::ActionRequest::MoveBackward:  s = "MoveBackward";  break;
                case common::ActionRequest::RotateLeft90:  s = "RotateLeft90";  break;
                case common::ActionRequest::RotateRight90: s = "RotateRight90"; break;
                case common::ActionRequest::RotateLeft45:  s = "RotateLeft45";  break;
                case common::ActionRequest::RotateRight45: s = "RotateRight45"; break;
                case common::ActionRequest::Shoot:         s = "Shoot";         break;
                case common::ActionRequest::GetBattleInfo: s = "GetBattleInfo"; break;
                default:                                  s = "DoNothing";     break;
            }
            if (ignored[k] &&
                (actions[k] == common::ActionRequest::MoveForward ||
                 actions[k] == common::ActionRequest::MoveBackward))
            {
                s += " (ignored)";
            }
            if (killedThisTurn[k]) {
                s += " (killed)";
            }
            oss << s;
        }
        if (k + 1 < N) oss << ", ";
    }
    return oss.str();
}

//------------------------------------------------------------------------------
// isGameOver / getResultString
//------------------------------------------------------------------------------
bool GameState::isGameOver() const {
    return gameOver_;
}

std::string GameState::getResultString() const {
    return resultStr_;
}

//------------------------------------------------------------------------------
// createSatelliteViewFor:
//   Returns a MySatelliteView built from the current board grid, marking (queryX,queryY) as '%'
//------------------------------------------------------------------------------
std::unique_ptr<common::SatelliteView>
GameState::createSatelliteViewFor(int queryX, int queryY) const
{
    return std::make_unique<MySatelliteView>(
        board_.getGrid(),
        rows_, cols_,
        queryX, queryY
    );
}

//------------------------------------------------------------------------------
// (A) Rotations
//------------------------------------------------------------------------------
void GameState::applyTankRotations(const std::vector<common::ActionRequest>& actions) {
    for (std::size_t k = 0; k < all_tanks_.size(); ++k) {
        if (!all_tanks_[k].alive) continue;
        auto act = actions[k];
        int& dir = all_tanks_[k].direction;
        switch (act) {
            case common::ActionRequest::RotateLeft90:
                dir = (dir + 6) % 8;
                break;
            case common::ActionRequest::RotateRight90:
                dir = (dir + 2) % 8;
                break;
            case common::ActionRequest::RotateLeft45:
                dir = (dir + 7) % 8;
                break;
            case common::ActionRequest::RotateRight45:
                dir = (dir + 1) % 8;
                break;
            default:
                break;
        }
    }
}

//------------------------------------------------------------------------------
// (B) Mines collision
//------------------------------------------------------------------------------
void GameState::handleTankMineCollisions() {
    for (auto& ts : all_tanks_) {
        if (!ts.alive) continue;
        if (board_.getCell(ts.x, ts.y) == '@') {
            ts.alive = false;
            board_.setCell(ts.x, ts.y, ' ');
        }
    }
}

//------------------------------------------------------------------------------
// (C) Cooldowns
//     (Not implemented in this version.)
//------------------------------------------------------------------------------
void GameState::updateTankCooldowns() {
    // No per‐tank cooldown in this version.
}

//------------------------------------------------------------------------------
// (D) Confirm backward‐move legality
//------------------------------------------------------------------------------
void GameState::confirmBackwardMoves(std::vector<bool>& ignored,
                                     const std::vector<common::ActionRequest>& actions)
{
    for (std::size_t k = 0; k < all_tanks_.size(); ++k) {
        if (!all_tanks_[k].alive) continue;
        if (actions[k] == common::ActionRequest::MoveBackward) {
            int backDir = (all_tanks_[k].direction + 4) & 7;
            int newX = static_cast<int>(all_tanks_[k].x) + Tank::DX[backDir];
            int newY = static_cast<int>(all_tanks_[k].y) + Tank::DY[backDir];
            if (newX < 0 || newY < 0 ||
                static_cast<std::size_t>(newX) >= rows_ ||
                static_cast<std::size_t>(newY) >= cols_ ||
                board_.getCell(newX, newY) == '#')
            {
                ignored[k] = true;
            }
        }
    }
}

//------------------------------------------------------------------------------
// (E) Update tank positions on the board (forward/backward)
//------------------------------------------------------------------------------
void GameState::updateTankPositionsOnBoard(std::vector<bool>& ignored,
                                           std::vector<bool>& killedThisTurn,
                                           const std::vector<common::ActionRequest>& actions)
{
    for (std::size_t k = 0; k < all_tanks_.size(); ++k) {
        if (!all_tanks_[k].alive) continue;
        auto act = actions[k];
        if (act != common::ActionRequest::MoveForward &&
            act != common::ActionRequest::MoveBackward)
        {
            continue;
        }

        int dir = all_tanks_[k].direction;
        if (act == common::ActionRequest::MoveBackward) {
            dir = (dir + 4) % 8;
        }
        int dx = 0, dy = 0;
        switch (dir) {
            case 0:  dx = -1; dy =  0; break;
            case 1:  dx = -1; dy =  1; break;
            case 2:           dy =  1; break;
            case 3:  dx = +1; dy =  1; break;
            case 4:  dx = +1;      break;
            case 5:  dx = +1; dy = -1; break;
            case 6:           dy = -1; break;
            case 7:  dx = -1; dy = -1; break;
            default: break;
        }

        if (ignored[k]) {
            continue;
        }

        int newX = static_cast<int>(all_tanks_[k].x) + dx;
        int newY = static_cast<int>(all_tanks_[k].y) + dy;
        if (newX < 0 || newY < 0 ||
            static_cast<std::size_t>(newX) >= rows_ ||
            static_cast<std::size_t>(newY) >= cols_)
        {
            ignored[k] = true;
            continue;
        }
        if (board_.getCell(newX, newY) == '#') {
            ignored[k] = true;
            continue;
        }

        // If the destination has a mine '@', kill the tank
        if (board_.getCell(newX, newY) == '@') {
            killedThisTurn[k] = true;
            all_tanks_[k].alive = false;
            board_.setCell(all_tanks_[k].x, all_tanks_[k].y, ' ');
            board_.setCell(newX, newY, ' ');
            continue;
        }

        // Legal move
        board_.setCell(all_tanks_[k].x, all_tanks_[k].y, ' ');
        all_tanks_[k].x = static_cast<std::size_t>(newX);
        all_tanks_[k].y = static_cast<std::size_t>(newY);
        board_.setCell(newX, newY, (all_tanks_[k].player_index == 1 ? '1' : '2'));
    }
}

//------------------------------------------------------------------------------
// (F) Advance all projectiles
//------------------------------------------------------------------------------
void GameState::advanceProjectiles() {
    for (auto& proj : projectiles_) {
        proj->advance();
    }
}

//------------------------------------------------------------------------------
// (G) Resolve projectile collisions
//------------------------------------------------------------------------------
void GameState::resolveProjectileCollisions(std::vector<bool>& killedThisTurn) {
    std::vector<std::unique_ptr<Projectile>> survivors;
    survivors.reserve(projectiles_.size());

    for (auto& proj : projectiles_) {
        std::size_t px = proj->getX();
        std::size_t py = proj->getY();

        // Out‐of‐bounds?
        if (px >= rows_ || py >= cols_) {
            continue;
        }
        char cell = board_.getCell(px, py);

        if (cell == '#') {
            // projectile hits a wall → destroy projectile
            continue;
        }
        if (cell == '@') {
            // mine is hidden under projectile, but projectile continues
            survivors.push_back(std::move(proj));
            continue;
        }
        if (cell == '1' || cell == '2') {
            // projectile hits a tank → kill that tank
            for (std::size_t kk = 0; kk < all_tanks_.size(); ++kk) {
                if (all_tanks_[kk].alive &&
                    all_tanks_[kk].x == px &&
                    all_tanks_[kk].y == py)
                {
                    killedThisTurn[kk] = true;
                    all_tanks_[kk].alive = false;
                    board_.setCell(px, py, ' ');
                    break;
                }
            }
            continue; // projectile is consumed
        }

        // Otherwise, no collision: projectile survives
        survivors.push_back(std::move(proj));
    }

    projectiles_.swap(survivors);
}

//------------------------------------------------------------------------------
// (H) Handle shooting
//------------------------------------------------------------------------------
void GameState::handleShooting(const std::vector<common::ActionRequest>& actions,
                               std::vector<bool>& killedThisTurn)
{
    for (std::size_t k = 0; k < all_tanks_.size(); ++k) {
        if (!all_tanks_[k].alive) continue;
        if (actions[k] != common::ActionRequest::Shoot) continue;
        if (all_tanks_[k].shells_left == 0) continue;

        // Decrement shells and spawn projectile:
        all_tanks_[k].shells_left--;
        std::size_t sx = all_tanks_[k].x;
        std::size_t sy = all_tanks_[k].y;
        int dir = all_tanks_[k].direction;

        projectiles_.push_back(
            std::make_unique<Projectile>(sx, sy, dir)
        );
    }
}

//------------------------------------------------------------------------------
// (I) Cleanup dead tanks & out‐of‐bounds projectiles
//------------------------------------------------------------------------------
void GameState::cleanupDestroyedEntities() {
    // Tanks: already removed from board_ when killed.
    // Projectiles: those off‐board or collided were removed above.
}

//------------------------------------------------------------------------------
// (J) Check game‐over conditions
//------------------------------------------------------------------------------
void GameState::checkGameEndConditions() {
    int alive1 = 0, alive2 = 0;
    for (auto& ts : all_tanks_) {
        if (!ts.alive) continue;
        if (ts.player_index == 1) ++alive1;
        else ++alive2;
    }

    if (alive1 == 0 && alive2 == 0) {
        gameOver_ = true;
        resultStr_ = "Tie, both players have zero tanks";
    }
    else if (alive1 == 0) {
        gameOver_ = true;
        resultStr_ = "Player 2 won with " + std::to_string(alive2) + " tanks still alive";
    }
    else if (alive2 == 0) {
        gameOver_ = true;
        resultStr_ = "Player 1 won with " + std::to_string(alive1) + " tanks still alive";
    }
    else if (currentStep_ + 1 >= maxSteps_) {
        gameOver_ = true;
        resultStr_ = "Tie, reached max steps = " + std::to_string(maxSteps_) +
                     ", player1 has " + std::to_string(alive1) +
                     ", player2 has " + std::to_string(alive2);
    }
}

void GameState::printBoard() const {
    const auto& grid = board_.getGrid();
    for (std::size_t r = 0; r < rows_; ++r) {
        for (std::size_t c = 0; c < cols_; ++c) {
            std::cout << grid[r][c];
        }
        std::cout << "\n";
    }
    std::cout << std::endl;
}
