#pragma once

#include <string>
#include <vector>
#include <memory>

#include "Board.h"
#include "Tank.h"
#include "Projectile.h"

#include "MyBattleInfo.h"
#include "MySatelliteView.h"

#include "common/ActionRequest.h"
#include "common/PlayerFactory.h"
#include "common/TankAlgorithmFactory.h"
#include "common/SatelliteView.h"

namespace arena {

/*
  The GameManager is the “top‐level” class that ties everything together.
  It:
    - reads the input file (board layout, maxSteps, numShells, dimensions)
    - constructs GameState, PlayerFactory, TankAlgorithmFactory
    - calls gameState.readBoard(...) and then gameState.run()
  The output is written to “output_<input_filename>.txt” per assignment spec.
*/

class GameManager {
public:
    GameManager(std::unique_ptr<common::PlayerFactory> pFac,
                std::unique_ptr<common::TankAlgorithmFactory> tFac);
    ~GameManager();

    void readBoard(const std::string& filename);
    void run();

private:
    int nextTankIndex_[3] = {0, 0, 0}; 
    struct TankState {
        int player_index;
        int tank_index;
        std::size_t x, y;
        int direction;
        bool alive;
        std::size_t shells_left;
    };

    // Map parameters:
    std::size_t rows_, cols_;
    std::size_t max_steps_, num_shells_;
    Board board_; 

    // All tanks in birth order, and a lookup map to find them:
    std::vector<TankState> all_tanks_;
    std::vector<std::vector<std::size_t>> tankIdMap_; 

    // One Player instance per side:
    std::unique_ptr<common::Player> player1_, player2_;
    std::unique_ptr<common::PlayerFactory> player_factory_;
    std::unique_ptr<common::TankAlgorithmFactory> tank_factory_;

    // One TankAlgorithm instance per tank (in birth order):
    std::vector<std::unique_ptr<common::TankAlgorithm>> all_tank_algorithms_;

    // Turn counters, shell starvation counters, etc.
    std::size_t current_turn_{0};
    std::size_t consecutive_no_shells_{0};

    void initializeTankIdMap();

    // Each turn, GameManager:
    //  1) collects each tank’s getAction() (handling GetBattleInfo)
    //  2) resolves rotations / moves / shoots / mines simultaneously
    //  3) writes one line to “output_<mapfile>.txt”
    //  4) checks win/tie conditions
    void applyRotations(const std::vector<common::ActionRequest>& actions);
    void applyMovements(const std::vector<common::ActionRequest>& actions,
                        std::vector<bool>& ignored,
                        std::vector<bool>& killedThisTurn);
    void applyShooting(const std::vector<common::ActionRequest>& actions,
                       std::vector<bool>& killedThisTurn);

    std::size_t findTankAt(std::size_t x, std::size_t y) const;
    std::size_t getGlobalTankIdx(int player_index, int tank_index) const {
        return tankIdMap_[player_index][tank_index];
    }
};

} // namespace arena
