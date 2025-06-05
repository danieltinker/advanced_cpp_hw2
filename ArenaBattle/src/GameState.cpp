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
{}

GameState::~GameState() = default;

//------------------------------------------------------------------------------
// initialize:
// 1) Copy board_, rows_, cols_, maxSteps_, num_shells_
// 2) Scan board for TANK1/TANK2 → build TankState + tankIdMap_
// 3) Create two Player instances via player_factory_->create(...)
// 4) Create one TankAlgorithm per TankState via tank_factory_->create(...)
// 5) Clear shells_, reset currentStep_, gameOver_, resultStr_
//------------------------------------------------------------------------------
void GameState::initialize(const Board& board, std::size_t maxSteps, std::size_t numShells) {
    board_ = board;
    rows_ = board.getRows();
    cols_ = board.getCols();

    maxSteps_ = maxSteps;
    num_shells_ = numShells;

    // Scan for tanks
    nextTankIndex_[1] = nextTankIndex_[2] = 0;
    all_tanks_.clear();
    tankIdMap_.assign(3, std::vector<std::size_t>(rows_ * cols_, SIZE_MAX));

    for (std::size_t r = 0; r < rows_; ++r) {
        for (std::size_t c = 0; c < cols_; ++c) {
            const Cell& cell = board_.getCell(static_cast<int>(c), static_cast<int>(r));
            if (cell.content == CellContent::TANK1 || cell.content == CellContent::TANK2) {
                int pidx = (cell.content == CellContent::TANK1 ? 1 : 2);
                int tidx = nextTankIndex_[pidx]++;
                TankState ts;
                ts.player_index = pidx;
                ts.tank_index = tidx;
                ts.x = static_cast<int>(c);
                ts.y = static_cast<int>(r);
                ts.direction = (pidx == 1 ? 6 : 2); // Player1 faces left(6), Player2 faces right(2)
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

    // Create Player1 and Player2
    player1_ = player_factory_->create(1,
                                       static_cast<std::size_t>(rows_),
                                       static_cast<std::size_t>(cols_),
                                       maxSteps_,
                                       num_shells_);
    player2_ = player_factory_->create(2,
                                       static_cast<std::size_t>(rows_),
                                       static_cast<std::size_t>(cols_),
                                       maxSteps_,
                                       num_shells_);

    // Create one TankAlgorithm per tank
    all_tank_algorithms_.clear();
    all_tank_algorithms_.reserve(all_tanks_.size());
    for (std::size_t k = 0; k < all_tanks_.size(); ++k) {
        int pidx = all_tanks_[k].player_index;
        int tidx = all_tanks_[k].tank_index;
        all_tank_algorithms_.push_back(
            tank_factory_->create(pidx, tidx)
        );
    }

    // Clear shells and bookkeeping
    shells_.clear();
    toRemove_.clear();
    positionMap_.clear();

    currentStep_ = 0;
    gameOver_ = false;
    resultStr_.clear();
}

//------------------------------------------------------------------------------
// advanceOneTurn:
//   Implements the 12 sub‐steps from HW1—a faithful two‐cell shell model.
//------------------------------------------------------------------------------
std::string GameState::advanceOneTurn() {
    if (gameOver_) return "";

    std::size_t N = all_tanks_.size();
    std::vector<common::ActionRequest> actions(N, common::ActionRequest::DoNothing);
    std::vector<bool> ignored(N, false);
    std::vector<bool> killedThisTurn(N, false);

    // --- (1) Gather each alive tank’s requested action ---
    for (std::size_t k = 0; k < N; ++k) {
        if (!all_tanks_[k].alive) continue;
        auto& alg = all_tank_algorithms_[k];

        common::ActionRequest req = alg->getAction();
        if (req == common::ActionRequest::GetBattleInfo) {
    // (a) Build a temporary char‐grid from board_.getGrid()
    std::vector<std::vector<char>> charGrid(rows_, std::vector<char>(cols_, ' '));
    for (int ry = 0; ry < static_cast<int>(rows_); ++ry) {
        for (int cx = 0; cx < static_cast<int>(cols_); ++cx) {
            const Cell& cell = board_.getCell(cx, ry);
            switch (cell.content) {
                case CellContent::WALL:  charGrid[ry][cx] = '#'; break;
                case CellContent::MINE:  charGrid[ry][cx] = '@'; break;
                case CellContent::TANK1: charGrid[ry][cx] = '1'; break;
                case CellContent::TANK2: charGrid[ry][cx] = '2'; break;
                default:                  charGrid[ry][cx] = ' '; break;
            }
        }
    }
    // (b) Mark the querying tank’s spot as '%':
    int qx = all_tanks_[k].x;
    int qy = all_tanks_[k].y;
    if (qx >= 0 && qx < static_cast<int>(cols_) &&
        qy >= 0 && qy < static_cast<int>(rows_))
    {
        charGrid[qy][qx] = '%';
    }

    // (c) Now construct MySatelliteView from charGrid
    MySatelliteView satView(
        charGrid,
        static_cast<int>(rows_),
        static_cast<int>(cols_),
        qx,
        qy
    );
    if (all_tanks_[k].player_index == 1) {
        player1_->updateTankWithBattleInfo(*alg, satView);
    } else {
        player2_->updateTankWithBattleInfo(*alg, satView);
    }
    // Get the “real” action next:
    req = alg->getAction();
}
        actions[k] = req;
    }

    // --- (2) Rotations ---
    applyTankRotations(actions);

    // --- (3) Mines collision (tank on '@') ---
    handleTankMineCollisions();

    // --- (4) Cooldowns (unused) ---
    updateTankCooldowns();

    // --- (5) Confirm backward move legality ---
    confirmBackwardMoves(ignored, actions);

    // --- (6) Move forward/backward (no wrap) ---
    updateTankPositionsOnBoard(ignored, killedThisTurn, actions);

    // --- (7) Handle shooting: spawn new shells with mid‐step check ---
    handleShooting(actions, killedThisTurn);

    // --- (8) Move all shells two sub‐steps each (wrap + mid‐step collisions) ---
    updateShellsWithOverrunCheck();

    // --- (9) Resolve shell collisions (shell vs shell, shell vs wall, shell vs tank) ---
    resolveShellCollisions();

    // --- (10) Cleanup dead entities (tanks already removed, shells flagged) ---
    cleanupDestroyedEntities();

    // --- (11) Check end‐of‐game conditions ---
    checkGameEndConditions();

    // --- (12) Increment turn counter ---
    ++currentStep_;

    // Build comma‐separated action string
    std::ostringstream oss;
    for (std::size_t k = 0; k < N; ++k) {
        if (!all_tanks_[k].alive) {
            oss << "killed";
        } else {
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
// createSatelliteViewFor: build MySatelliteView with '%' at (queryX,queryY)
//------------------------------------------------------------------------------
std::unique_ptr<common::SatelliteView>
GameState::createSatelliteViewFor(int queryX, int queryY) const
{
    // Build a char‐grid snapshot
    std::vector<std::vector<char>> charGrid(rows_, std::vector<char>(cols_, ' '));
    for (int ry = 0; ry < static_cast<int>(rows_); ++ry) {
        for (int cx = 0; cx < static_cast<int>(cols_); ++cx) {
            const Cell& cell = board_.getCell(cx, ry);
            switch (cell.content) {
                case CellContent::WALL:  charGrid[ry][cx] = '#'; break;
                case CellContent::MINE:  charGrid[ry][cx] = '@'; break;
                case CellContent::TANK1: charGrid[ry][cx] = '1'; break;
                case CellContent::TANK2: charGrid[ry][cx] = '2'; break;
                default:                  charGrid[ry][cx] = ' '; break;
            }
        }
    }
    // Mark the querying tank spot as '%'
    if (queryX >= 0 && queryX < static_cast<int>(cols_) &&
        queryY >= 0 && queryY < static_cast<int>(rows_))
    {
        charGrid[queryY][queryX] = '%';
    }

    return std::make_unique<MySatelliteView>(
        charGrid,
        static_cast<int>(rows_),
        static_cast<int>(cols_),
        queryX,
        queryY
    );
}


//------------------------------------------------------------------------------
// (2) applyTankRotations: change each tank’s direction on rotate request
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
// (3) handleTankMineCollisions: kill any tank on a mine
//------------------------------------------------------------------------------
void GameState::handleTankMineCollisions() {
    for (auto& ts : all_tanks_) {
        if (!ts.alive) continue;
        Cell& cell = board_.getCell(ts.x, ts.y);
        if (cell.content == CellContent::MINE) {
            ts.alive = false;
            cell.content = CellContent::EMPTY;
        }
    }
}

//------------------------------------------------------------------------------
// (4) updateTankCooldowns (unused)
//------------------------------------------------------------------------------
void GameState::updateTankCooldowns() {
    // No per‐tank cooldown in this version
}

//------------------------------------------------------------------------------
// (5) confirmBackwardMoves: mark ignored if backward into wall or OOB
//------------------------------------------------------------------------------
void GameState::confirmBackwardMoves(std::vector<bool>& ignored,
                                     const std::vector<common::ActionRequest>& actions)
{
    for (std::size_t k = 0; k < all_tanks_.size(); ++k) {
        if (!all_tanks_[k].alive) continue;
        if (actions[k] == common::ActionRequest::MoveBackward) {
            int backDir = (all_tanks_[k].direction + 4) & 7;
            int dx = 0, dy = 0;
            switch (backDir) {
                case 0:  dy = -1; break;    // Up
                case 1:  dx = +1; dy = -1; break; // Up‐Right
                case 2:  dx = +1; break;    // Right
                case 3:  dx = +1; dy = +1; break; // Down‐Right
                case 4:  dy = +1; break;    // Down
                case 5:  dx = -1; dy = +1; break; // Down‐Left
                case 6:  dx = -1; break;    // Left
                case 7:  dx = -1; dy = -1; break; // Up‐Left
            }
            int newX = all_tanks_[k].x + dx;
            int newY = all_tanks_[k].y + dy;
            if (newX < 0 || newY < 0 ||
                newX >= board_.getWidth() || newY >= board_.getHeight() ||
                board_.getCell(newX, newY).content == CellContent::WALL)
            {
                ignored[k] = true;
            }
        }
    }
}

//------------------------------------------------------------------------------
// (6) updateTankPositionsOnBoard: move forward/backward, no wrap.
//------------------------------------------------------------------------------
void GameState::updateTankPositionsOnBoard(std::vector<bool>& ignored,
                                           std::vector<bool>& killedThisTurn,
                                           const std::vector<common::ActionRequest>& actions)
{
    // Clear old tank marks:
    board_.clearTankMarks();

    for (std::size_t k = 0; k < all_tanks_.size(); ++k) {
        if (!all_tanks_[k].alive) continue;
        auto act = actions[k];
        if (act != common::ActionRequest::MoveForward &&
            act != common::ActionRequest::MoveBackward)
        {
            continue;
        }
        if (ignored[k]) continue;

        int dx = 0, dy = 0;
        int moveDir = all_tanks_[k].direction;
        if (act == common::ActionRequest::MoveBackward) {
            moveDir = (moveDir + 4) & 7;
        }
        switch (moveDir) {
            case 0:  dy = -1; break;    // Up
            case 1:  dx = +1; dy = -1; break; // Up‐Right
            case 2:  dx = +1; break;    // Right
            case 3:  dx = +1; dy = +1; break; // Down‐Right
            case 4:  dy = +1; break;    // Down
            case 5:  dx = -1; dy = +1; break; // Down‐Left
            case 6:  dx = -1; break;    // Left
            case 7:  dx = -1; dy = -1; break; // Up‐Left
        }
        int newX = all_tanks_[k].x + dx;
        int newY = all_tanks_[k].y + dy;

        // If out of bounds or wall, ignore:
        if (newX < 0 || newY < 0 ||
            newX >= board_.getWidth() || newY >= board_.getHeight() ||
            board_.getCell(newX, newY).content == CellContent::WALL)
        {
            ignored[k] = true;
            continue;
        }

        // If stepping onto a mine:
        if (board_.getCell(newX, newY).content == CellContent::MINE) {
            killedThisTurn[k] = true;
            all_tanks_[k].alive = false;
            board_.setCell(all_tanks_[k].x, all_tanks_[k].y, CellContent::EMPTY);
            board_.setCell(newX, newY, CellContent::EMPTY);
            continue;
        }

        // Normal move:
        board_.setCell(all_tanks_[k].x, all_tanks_[k].y, CellContent::EMPTY);
        all_tanks_[k].x = newX;
        all_tanks_[k].y = newY;
        board_.setCell(newX, newY,
                       (all_tanks_[k].player_index == 1 ? CellContent::TANK1 : CellContent::TANK2));
    }
}

//------------------------------------------------------------------------------
// (7) handleShooting: spawn new shells at (x+dx,y+dy) with immediate collision.
//------------------------------------------------------------------------------
void GameState::handleShooting(const std::vector<common::ActionRequest>& actions,
                               std::vector<bool>& /*killedThisTurn*/) 
{
    auto spawnShell = [&](TankState& ts) {
        int sx = ts.x, sy = ts.y;
        int dx = 0, dy = 0;
        switch (ts.direction) {
            case 0:  dy = -1; break;    // Up
            case 1:  dx = +1; dy = -1; break; // Up‐Right
            case 2:  dx = +1; break;    // Right
            case 3:  dx = +1; dy = +1; break; // Down‐Right
            case 4:  dy = +1; break;    // Down
            case 5:  dx = -1; dy = +1; break; // Down‐Left
            case 6:  dx = -1; break;    // Left
            case 7:  dx = -1; dy = -1; break; // Up‐Left
        }
        int spawnX = (sx + dx + board_.getWidth())  % board_.getWidth();
        int spawnY = (sy + dy + board_.getHeight()) % board_.getHeight();

        // Mid‐step collision at spawn cell:
        if (!handleShellMidStepCollision(spawnX, spawnY)) {
            shells_.push_back({ spawnX, spawnY, ts.direction });
        }
    };

    for (std::size_t k = 0; k < all_tanks_.size(); ++k) {
        if (!all_tanks_[k].alive) continue;
        if (actions[k] != common::ActionRequest::Shoot) continue;
        if (all_tanks_[k].shells_left == 0) continue;

        all_tanks_[k].shells_left--;
        spawnShell(all_tanks_[k]);
    }
}

//------------------------------------------------------------------------------
// (8) updateShellsWithOverrunCheck: move each shell two sub‐steps (wrap+mid‐check)
//------------------------------------------------------------------------------
void GameState::updateShellsWithOverrunCheck() {
    toRemove_.clear();
    positionMap_.clear();
    board_.clearShellMarks();

    for (std::size_t i = 0; i < shells_.size(); ++i) {
        int dx = 0, dy = 0;
        switch (shells_[i].dir) {
            case 0:  dy = -1; break;    // Up
            case 1:  dx = +1; dy = -1; break; // Up‐Right
            case 2:  dx = +1; break;    // Right
            case 3:  dx = +1; dy = +1; break; // Down‐Right
            case 4:  dy = +1; break;    // Down
            case 5:  dx = -1; dy = +1; break; // Down‐Left
            case 6:  dx = -1; break;    // Left
            case 7:  dx = -1; dy = -1; break; // Up‐Left
        }

        for (int step = 0; step < 2; ++step) {
            int nextX = shells_[i].x + dx;
            int nextY = shells_[i].y + dy;
            board_.wrapCoords(nextX, nextY);

            if (handleShellMidStepCollision(nextX, nextY)) {
                toRemove_.insert(i);
                break;
            }
            shells_[i].x = nextX;
            shells_[i].y = nextY;
            positionMap_[{ nextX, nextY }].push_back(i);
        }
    }
}

//------------------------------------------------------------------------------
// (9) resolveShellCollisions: any cell with ≥2 shells → all shells removed;
// mark survivors’ hasShellOverlay=true.
//------------------------------------------------------------------------------
void GameState::resolveShellCollisions() {
    // A) Mutually destroy shells that share a cell
    for (auto const& entry : positionMap_) {
        const auto& idxs = entry.second;
        if (idxs.size() > 1) {
            for (auto idx : idxs) {
                toRemove_.insert(idx);
            }
        }
    }

    // B) Build survivors and mark overlay on board
    std::vector<Shell> survivors;
    survivors.reserve(shells_.size());
    for (std::size_t i = 0; i < shells_.size(); ++i) {
        if (toRemove_.count(i) == 0) {
            board_.getCell(shells_[i].x, shells_[i].y).hasShellOverlay = true;
            survivors.push_back(shells_[i]);
        }
    }
    shells_.swap(survivors);
}

//------------------------------------------------------------------------------
// (I) handleShellMidStepCollision: wall/tank/mine logic at (x,y)
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
    // 2) TANK1?
    if (cell.content == CellContent::TANK1) {
        for (auto& ts : all_tanks_) {
            if (ts.alive && ts.x == x && ts.y == y && ts.player_index == 1) {
                ts.alive = false;
                break;
            }
        }
        cell.content = CellContent::EMPTY;
        return true;
    }
    // 3) TANK2?
    if (cell.content == CellContent::TANK2) {
        for (auto& ts : all_tanks_) {
            if (ts.alive && ts.x == x && ts.y == y && ts.player_index == 2) {
                ts.alive = false;
                break;
            }
        }
        cell.content = CellContent::EMPTY;
        return true;
    }
    // 4) MINE? shell passes through
    if (cell.content == CellContent::MINE) {
        return false;
    }
    // 5) Otherwise (EMPTY or overlay), no destruction
    return false;
}

//------------------------------------------------------------------------------
// (10) cleanupDestroyedEntities: nothing to do (tanks removed, shells removed)
//------------------------------------------------------------------------------
void GameState::cleanupDestroyedEntities() {
    // Everything is already removed in earlier steps.
}

//------------------------------------------------------------------------------
// (11) checkGameEndConditions
//------------------------------------------------------------------------------
void GameState::checkGameEndConditions() {
    int alive1 = 0, alive2 = 0;
    for (auto const& ts : all_tanks_) {
        if (ts.alive) {
            if (ts.player_index == 1) ++alive1;
            else ++alive2;
        }
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

//------------------------------------------------------------------------------
// (12) printBoard: overlay shells (*) and colored tank arrows
//------------------------------------------------------------------------------
void GameState::printBoard() const {
    // Copy the grid (Cells) so we can overlay shells/tanks without mutating original
    auto gridCopy = board_.getGrid();

    // Tanks overlay: place placeholders so we can fetch direction
    for (auto const& ts : all_tanks_) {
        if (!ts.alive) continue;
        int r = ts.y, c = ts.x;
        if (r >= 0 && r < static_cast<int>(rows_) && c >= 0 && c < static_cast<int>(cols_)) {
            gridCopy[r][c].content =
                (ts.player_index == 1 ? CellContent::TANK1 : CellContent::TANK2);
        }
    }

    // Now print row by row:
    for (int r = 0; r < static_cast<int>(rows_); ++r) {
        for (int c = 0; c < static_cast<int>(cols_); ++c) {
            const Cell& cell = gridCopy[r][c];
            if (cell.content == CellContent::WALL) {
                std::cout << "#";
            }
            else if (cell.content == CellContent::MINE) {
                std::cout << "@";
            }
            else if (cell.content == CellContent::EMPTY) {
                if (cell.hasShellOverlay) {
                    std::cout << "*";
                } else {
                    std::cout << " ";
                }
            }
            else if (cell.content == CellContent::TANK1 ||
                     cell.content == CellContent::TANK2)
            {
                // Find that tank’s direction
                int dir = 0;
                int pidx = (cell.content == CellContent::TANK1 ? 1 : 2);
                for (auto const& ts : all_tanks_) {
                    if (ts.alive &&
                        ts.y == r &&
                        ts.x == c &&
                        ts.player_index == pidx)
                    {
                        dir = ts.direction;
                        break;
                    }
                }
                const char* arrow = directionToArrow(dir);
                if (cell.content == CellContent::TANK1) {
                    // Red arrow
                    std::cout << "\033[31m" << arrow << "\033[0m";
                } else {
                    // Blue arrow
                    std::cout << "\033[34m" << arrow << "\033[0m";
                }
            }
            else {
                std::cout << "?";
            }
        }
        std::cout << "\n";
    }
    std::cout << std::endl;
}

//------------------------------------------------------------------------------
// directionToArrow: map 0..7 to a UTF‐8 arrow
//------------------------------------------------------------------------------
const char* GameState::directionToArrow(int dir) {
    switch (dir & 7) {
        case 0: return "↑";
        case 1: return "↗";
        case 2: return "→";
        case 3: return "↘";
        case 4: return "↓";
        case 5: return "↙";
        case 6: return "←";
        case 7: return "↖";
        default: return "?";
    }
}
