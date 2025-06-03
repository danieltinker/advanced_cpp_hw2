#include "GameManager.h"
#include "Tank.h"
#include "utils.h"
#include <fstream>
#include <sstream>
#include <iostream>

using namespace arena;
using namespace common;

GameManager::GameManager(std::unique_ptr<PlayerFactory> pFac,
                         std::unique_ptr<TankAlgorithmFactory> tFac)
    : player_factory_(std::move(pFac)),
      tank_factory_(std::move(tFac))
{
    rows_ = cols_ = max_steps_ = num_shells_ = 0;
    nextTankIndex_[1] = nextTankIndex_[2] = 0;
}

GameManager::~GameManager() = default;

void GameManager::readBoard(const std::string& filename) {
    std::ifstream in(filename);
    if (!in) {
        std::cerr << "Cannot open map file: " << filename << "\n";
        std::exit(1);
    }
    std::string line;

    // 1) map name (ignore)
    std::getline(in, line);

    // 2) MaxSteps = <NUM>
    std::getline(in, line);
    parseKeyValue(line, "MaxSteps", max_steps_);

    // 3) NumShells = <NUM>
    std::getline(in, line);
    parseKeyValue(line, "NumShells", num_shells_);

    // 4) Rows = <NUM>
    std::getline(in, line);
    parseKeyValue(line, "Rows", rows_);

    // 5) Cols = <NUM>
    std::getline(in, line);
    parseKeyValue(line, "Cols", cols_);

    // Initialize board_ as rows × cols filled with ' '
    board_ = Board(rows_, cols_);

    // Read board lines:
    std::size_t r = 0;
    while (r < rows_ && std::getline(in, line)) {
        for (std::size_t c = 0; c < cols_ && c < line.size(); ++c) {
            char ch = line[c];
            if (ch == '#' || ch == '@' || ch == '1' || ch == '2') {
                board_.setCell(r, c, ch);
            } else {
                board_.setCell(r, c, ' ');
            }
        }
        ++r;
    }

    // 6) Scan for tanks:
    nextTankIndex_[1] = nextTankIndex_[2] = 0;
    all_tanks_.clear();
    tankIdMap_.assign(3, std::vector<std::size_t>(rows_ * cols_, SIZE_MAX));

    for (std::size_t i = 0; i < rows_; ++i) {
        for (std::size_t j = 0; j < cols_; ++j) {
            char ch = board_.getCell(i, j);
            if (ch == '1' || ch == '2') {
                int pidx = (ch == '1') ? 1 : 2;
                int tidx = nextTankIndex_[pidx]++;
                TankState ts;
                ts.player_index = pidx;
                ts.tank_index = tidx;
                ts.x = i; ts.y = j;
                ts.direction = (pidx == 1 ? 6 : 2);
                ts.alive = true;
                ts.shells_left = num_shells_;
                all_tanks_.push_back(ts);

                std::size_t globalIdx = all_tanks_.size() - 1;
                if (static_cast<std::size_t>(tidx) >= tankIdMap_[pidx].size()) {
                    tankIdMap_[pidx].resize(tidx + 1, SIZE_MAX);
                }
                tankIdMap_[pidx][tidx] = globalIdx;
            }
        }
    }

    // 7) Create the two Player objects
    player1_ = player_factory_->create(1, max_steps_, num_shells_);
    player2_ = player_factory_->create(2, max_steps_, num_shells_);

    // 8) Create one TankAlgorithm per tank in birth order
    all_tank_algorithms_.clear();
    all_tank_algorithms_.reserve(all_tanks_.size());
    for (std::size_t k = 0; k < all_tanks_.size(); ++k) {
        int pidx = all_tanks_[k].player_index;
        int tidx = all_tanks_[k].tank_index;
        all_tank_algorithms_.push_back(
            tank_factory_->create(pidx, tidx)
        );
    }

    // Initialize game‐over flags
    current_turn_ = 0;
    consecutive_no_shells_ = 0;
}

void GameManager::run() {
    // Expect: tanks_game <input_file>
    // Output to: output_<input_file>.txt
    std::string outFile = "output_" + std::to_string(current_turn_) + ".txt"; 
    // (You’ll want to replace current_turn_ with the actual input filename string)
    // For brevity, we’ll just write to “output.txt”.
    std::ofstream out("output.txt");
    if (!out) {
        std::cerr << "Cannot open output.txt\n";
        return;
    }

    while (current_turn_ < max_steps_) {
        // 1) Count alive tanks per side
        int alive1 = 0, alive2 = 0;
        for (auto& ts : all_tanks_) {
            if (!ts.alive) continue;
            if (ts.player_index == 1) ++alive1;
            else ++alive2;
        }
        if (alive1 == 0 && alive2 == 0) {
            out << "Tie, both players have zero tanks\n";
            return;
        }
        if (alive1 == 0) {
            out << "Player 2 won with " << alive2 << " tanks still alive\n";
            return;
        }
        if (alive2 == 0) {
            out << "Player 1 won with " << alive1 << " tanks still alive\n";
            return;
        }

        // 2) Collect each tank’s requested action
        std::vector<common::ActionRequest> actions(all_tanks_.size(), common::ActionRequest::DoNothing);
        std::vector<bool> ignored(all_tanks_.size(), false);
        std::vector<bool> killedThisTurn(all_tanks_.size(), false);

        for (std::size_t k = 0; k < all_tanks_.size(); ++k) {
            if (!all_tanks_[k].alive) continue;
            auto& tankAlg = all_tank_algorithms_[k];
            common::ActionRequest req = tankAlg->getAction();
            if (req == common::ActionRequest::GetBattleInfo) {
                // Build a MySatelliteView for this tank:
                MyBattleInfo info;
                info.rows = rows_;
                info.cols = cols_;
                info.grid = board_.getGrid();

                // Mark the querying tank’s spot as '%':
                info.grid[all_tanks_[k].x][all_tanks_[k].y] = '%';

                tankAlg->updateBattleInfo(info);
                req = tankAlg->getAction();
            }
            actions[k] = req;
        }

        // 3a) Rotations
        applyRotations(actions);

        // 3b) Movements (forward/backward) & mines
        applyMovements(actions, ignored, killedThisTurn);

        // 3c) Shooting
        applyShooting(actions, killedThisTurn);

        // 4) Write one line in output.txt
        std::vector<std::string> tokens;
        tokens.reserve(all_tanks_.size());
        for (std::size_t k = 0; k < all_tanks_.size(); ++k) {
            if (!all_tanks_[k].alive) {
                if (killedThisTurn[k]) {
                    tokens.push_back(actionToString(actions[k]) + " (killed)");
                } else {
                    tokens.push_back("killed");
                }
            } else {
                std::string s = actionToString(actions[k]);
                if (ignored[k]) s += " (ignored)";
                tokens.push_back(s);
            }
        }
        for (std::size_t i = 0; i < tokens.size(); ++i) {
            out << tokens[i] << (i + 1 < tokens.size() ? ", " : "\n");
        }

        // 5) Shell starvation tie
        int shells1 = 0, shells2 = 0;
        for (auto& ts : all_tanks_) {
            if (!ts.alive) continue;
            if (ts.player_index == 1) shells1 += static_cast<int>(ts.shells_left);
            else shells2 += static_cast<int>(ts.shells_left);
        }
        if (shells1 == 0 && shells2 == 0) {
            ++consecutive_no_shells_;
            if (consecutive_no_shells_ >= 40) {
                out << "Tie, both players have zero shells for 40 steps\n";
                return;
            }
        } else {
            consecutive_no_shells_ = 0;
        }

        current_turn_++;
    }

    // Tie if maxSteps reached
    int alive1 = 0, alive2 = 0;
    for (auto& ts : all_tanks_) {
        if (ts.alive) {
            if (ts.player_index == 1) ++alive1;
            else ++alive2;
        }
    }
    out << "Tie, reached max steps = " << max_steps_
        << ", player1 has " << alive1
        << ", player2 has " << alive2 << "\n";
}

void GameManager::applyRotations(const std::vector<common::ActionRequest>& actions) {
    for (std::size_t k = 0; k < all_tanks_.size(); ++k) {
        if (!all_tanks_[k].alive) continue;
        auto act = actions[k];
        auto& dir = all_tanks_[k].direction;
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

void GameManager::applyMovements(const std::vector<common::ActionRequest>& actions,
                                 std::vector<bool>& ignored,
                                 std::vector<bool>& killedThisTurn)
{
    for (std::size_t k = 0; k < all_tanks_.size(); ++k) {
        if (!all_tanks_[k].alive) continue;
        auto act = actions[k];
        if (act != common::ActionRequest::MoveForward &&
            act != common::ActionRequest::MoveBackward) continue;

        int dir = all_tanks_[k].direction;
        if (act == common::ActionRequest::MoveBackward) {
            dir = (dir + 4) % 8;
        }

        int dx = 0, dy = 0;
        switch (dir) {
            case 0:  dx = -1; dy =  0; break;
            case 1:  dx = -1; dy =  1; break;
            case 2:           dy =  1; break;
            case 3:  dx =  1; dy =  1; break;
            case 4:  dx =  1;      break;
            case 5:  dx =  1; dy = -1; break;
            case 6:           dy = -1; break;
            case 7:  dx = -1; dy = -1; break;
            default: break;
        }

        std::size_t newx = all_tanks_[k].x + dx;
        std::size_t newy = all_tanks_[k].y + dy;
        if (newx >= rows_ || newy >= cols_ || board_.getCell(newx, newy) == '#') {
            ignored[k] = true;
            continue;
        }

        if (board_.getCell(newx, newy) == '@') {
            killedThisTurn[k] = true;
            all_tanks_[k].alive = false;
            board_.setCell(all_tanks_[k].x, all_tanks_[k].y, ' ');
            board_.setCell(newx, newy, ' ');
            continue;
        }

        // Normal move
        board_.setCell(all_tanks_[k].x, all_tanks_[k].y, ' ');
        all_tanks_[k].x = newx;
        all_tanks_[k].y = newy;
        board_.setCell(newx, newy, (all_tanks_[k].player_index == 1 ? '1' : '2'));
    }
}

void GameManager::applyShooting(const std::vector<common::ActionRequest>& actions,
                                std::vector<bool>& killedThisTurn)
{
    for (std::size_t k = 0; k < all_tanks_.size(); ++k) {
        if (!all_tanks_[k].alive) continue;
        if (actions[k] != common::ActionRequest::Shoot) continue;
        if (all_tanks_[k].shells_left == 0) continue;

        all_tanks_[k].shells_left--;
        auto x = all_tanks_[k].x;
        auto y = all_tanks_[k].y;
        int dir = all_tanks_[k].direction;
        int dx = 0, dy = 0;
        switch (dir) {
            case 0:  dx = -1; dy =  0; break;
            case 1:  dx = -1; dy =  1; break;
            case 2:           dy =  1; break;
            case 3:  dx =  1; dy =  1; break;
            case 4:  dx =  1;      break;
            case 5:  dx =  1; dy = -1; break;
            case 6:           dy = -1; break;
            case 7:  dx = -1; dy = -1; break;
            default: break;
        }

        std::size_t sx = x, sy = y;
        while (true) {
            sx = sx + dx;
            sy = sy + dy;
            if (sx >= rows_ || sy >= cols_) break;
            char at = board_.getCell(sx, sy);
            if (at == '#') break;
            if (at == '@') {
                // shell hides the mine, continues
                continue;
            }
            if (at == '1' || at == '2') {
                auto victim = findTankAt(sx, sy);
                if (victim != SIZE_MAX && all_tanks_[victim].alive) {
                    killedThisTurn[victim] = true;
                    all_tanks_[victim].alive = false;
                    board_.setCell(sx, sy, ' ');
                }
                break;
            }
        }
    }
}

std::size_t GameManager::findTankAt(std::size_t x, std::size_t y) const {
    for (std::size_t k = 0; k < all_tanks_.size(); ++k) {
        if (all_tanks_[k].alive && all_tanks_[k].x == x && all_tanks_[k].y == y) {
            return k;
        }
    }
    return SIZE_MAX;
}
