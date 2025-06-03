#include "GameState.h"
#include <fstream>
#include <sstream>
#include <iostream>

namespace arena{


// using namespace common;

GameState::GameState() = default;
GameState::~GameState() = default;

void GameState::initialize(const Board& board, std::size_t maxSteps, std::size_t numShells) {
    // 1) Copy the board
    board_ = board;

    // 2) Store maxSteps, numShells, rows, cols
    maxSteps_ = maxSteps;
    num_shells_ = numShells;
    rows_ = board.getRows();
    cols_ = board.getCols();

    // 3) Scan the board for '1' or '2' → spawn TankState and TankAlgorithm
    nextTankIndex_[1] = nextTankIndex_[2] = 0;
    all_tanks_.clear();
    tankIdMap_.assign(3, std::vector<std::size_t>(rows_ * cols_, SIZE_MAX));

    // First pass: create TankState entries:
    for (std::size_t r = 0; r < rows_; ++r) {
        for (std::size_t c = 0; c < cols_; ++c) {
            char ch = board.getCell(r, c);
            if (ch == '1' || ch == '2') {
                int pidx = (ch == '1') ? 1 : 2;
                int tidx = nextTankIndex_[pidx]++;
                TankState ts;
                ts.player_index = pidx;
                ts.tank_index = tidx;
                ts.x = r;
                ts.y = c;
                ts.direction = (pidx == 1 ? 6 : 2); // Left for P1, Right for P2
                ts.alive = true;
                ts.shells_left = numShells;
                all_tanks_.push_back(ts);

                std::size_t globalIdx = all_tanks_.size() - 1;
                if (static_cast<std::size_t>(tidx) >= tankIdMap_[pidx].size()) {
                    tankIdMap_[pidx].resize(tidx + 1, SIZE_MAX);
                }
                tankIdMap_[pidx][tidx] = globalIdx;
            }
        }
    }

    // 4) Create Player1, Player2 using player_factory_
    player1_ = player_factory_->create(1, maxSteps_, numShells);
    player2_ = player_factory_->create(2, maxSteps_, numShells);

    // 5) For each TankState, create a TankAlgorithm:
    all_tank_algorithms_.clear();
    all_tank_algorithms_.reserve(all_tanks_.size());
    for (std::size_t k = 0; k < all_tanks_.size(); ++k) {
        int pidx = all_tanks_[k].player_index;
        int tidx = all_tanks_[k].tank_index;
        all_tank_algorithms_.push_back(
            tank_factory_->create(pidx, tidx)
        );
    }

    // Game is not over yet:
    currentStep_ = 0;
    gameOver_ = false;
    resultStr_.clear();
    // TODO: consecutive_no_shells_ = 0;   fix logic
}

void GameState::step(common::ActionRequest action1, common::ActionRequest action2) {
    // 1) apply each tank’s rotations:
    applyTankActions(action1, action2);

    // 2) handle collisions with mines
    handleTankMineCollisions();

    // 3) update any cooldowns (if you have a “cooldown” concept)
    updateTankCooldowns();

    // 4) confirm backward‐move legality
    confirmBackwardMoves();

    // 5) update each tank’s position on board_ (i.e. actually move them)
    updateTankPositionsOnBoard();

    // 6) advance each projectile by one step
    advanceProjectiles();

    // 7) resolve any projectile‐projectile collisions or projectile‐wall
    resolveProjectileCollisions();

    // 8) handle shooting actions (spawn new shells)
    handleShooting(action1, action2);

    // 9) remove dead entities (tanks that died, mines that exploded, projectiles off‐board)
    cleanupDestroyedEntities();

    // 10) check end‐of‐game conditions (win/tie)
    checkGameEndConditions(action1, action2);

    currentStep_++;
}

bool GameState::isGameOver() const {
    return gameOver_;
}

std::string GameState::getResultString() const {
    return resultStr_;
}

std::unique_ptr<common::SatelliteView> GameState::createSatelliteView() const {
    // Build a MySatelliteView snapshot of “board_.getGrid()”,
    // marking whichever tank requested info with '%'. 
    // (Here we assume a single caller; in reality GM will pass the correct query_x, query_y.)
    // For demonstration, let’s pick the first alive tank as “querying tank”:
    std::size_t qx = 0, qy = 0;
    for (auto& ts : all_tanks_) {
        if (ts.alive) { qx = ts.x; qy = ts.y; break; }
    }
    return std::make_unique<MySatelliteView>(board_.getGrid(), rows_, cols_, qx, qy);
}

void GameState::applyTankActions(common::ActionRequest action1, common::ActionRequest action2) {
    // Apply rotations first:
    for (int k = 0; k < all_tanks_.size(); ++k) {
        if (!all_tanks_[k].alive) continue;
        common::ActionRequest act = (k == 0 ? action1 : action2);
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

void GameState::handleTankMineCollisions() {
    // If a tank moved onto '@', kill it:
    for (auto& ts : all_tanks_) {
        if (!ts.alive) continue;
        if (board_.getCell(ts.x, ts.y) == '@') {
            ts.alive = false;
            board_.setCell(ts.x, ts.y, ' ');
        }
    }
}

void GameState::updateTankCooldowns() {
    // If you have any cooldown logic, implement here. Otherwise, skip.
}

void GameState::confirmBackwardMoves() {
    // If you mark “wanted to move backward” earlier, confirm legality here.
}

void GameState::updateTankPositionsOnBoard() {
    // Move forward/backward actions. For each tank:
    //   - compute new (nx,ny)
    //   - if out‐of‐bounds or wall '#', ignore
    //   - else update board_ and ts.x, ts.y
}

void GameState::advanceProjectiles() {
    for (auto& proj : projectiles_) {
        proj->advance();
    }
}

void GameState::resolveProjectileCollisions() {
    // For each projectile, if hits wall '#', remove it.
    // If it hits tank ‘ts’, kill that tank and remove projectile.
}

void GameState::handleShooting(common::ActionRequest action1, common::ActionRequest action2) {
    // If action == Shoot, spawn a new Projectile at (ts.x, ts.y) with direction ts.direction,
    // decrement ts.shells_left if >0. GM will enforce shells_left_ > 0.
}

void GameState::cleanupDestroyedEntities() {
    // Remove any projectiles off‐board or that collided. Remove any tanks that died.
}

void GameState::checkGameEndConditions(common::ActionRequest action1, common::ActionRequest action2) {
    int alive1 = 0, alive2 = 0;
    for (auto& ts : all_tanks_) {
        if (ts.alive) {
            if (ts.player_index == 1) ++alive1;
            else ++alive2;
        }
    }
    if (alive1 == 0 && alive2 == 0) {
        gameOver_ = true;
        resultStr_ = "Tie, both players have zero tanks";
    } else if (alive1 == 0) {
        gameOver_ = true;
        resultStr_ = "Player 2 won with " + std::to_string(alive2) + " tanks still alive";
    } else if (alive2 == 0) {
        gameOver_ = true;
        resultStr_ = "Player 1 won with " + std::to_string(alive1) + " tanks still alive";
    } else if (currentStep_ + 1 >= maxSteps_) {
        gameOver_ = true;
        resultStr_ = "Tie, reached max steps = " + std::to_string(maxSteps_) +
                     ", player1 has " + std::to_string(alive1) +
                     ", player2 has " + std::to_string(alive2);
    }
}

}//namespace arena

