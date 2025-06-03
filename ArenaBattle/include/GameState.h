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
#include "common/SatelliteView.h"

namespace arena {

/*
  GameState keeps track of:
    - Board (walls, mines, tanks, etc.)
    - A vector of TankState (one per tank in birth order)
    - A vector of Projectile (shells in flight)
    - maxSteps, currentStep, gameOver flag, result string

  It provides:
    - initialize(...) to load Board, maxSteps, numShells, spawn Tank instances
    - step(action1, action2) to process two tanks’ moves each tick
    - createSatelliteView() so that a requesting tank can see a 2D snapshot
*/
class GameState {
public:
    GameState();
    ~GameState();

    /// Initialize from a parsed Board, maxSteps, and shells per tank.
    void initialize(const Board& board, std::size_t maxSteps, std::size_t numShells);

    /// Advance one tick using two actions (one per tank), in birth order.
    void step(common::ActionRequest action1, common::ActionRequest action2);

    bool isGameOver() const;
    std::string getResultString() const;

    /// Returns a SatelliteView (snapshot) for the AI.
    std::unique_ptr<common::SatelliteView> createSatelliteView() const;

private:
    struct TankState {
        int player_index;       // 1 or 2
        int tank_index;         // 0-based index within that player
        std::size_t x, y;       // current board coordinates
        int direction;          // 0..7 (0=Up,1=Up-Right,...,6=Left,7=Up-Left)
        bool alive;
        std::size_t shells_left;
    };

    // Internal helpers (called in sequence by step()):
    void applyTankActions(common::ActionRequest action1, common::ActionRequest action2);
    void handleTankMineCollisions();
    void updateTankCooldowns();
    void confirmBackwardMoves();
    void updateTankPositionsOnBoard();
    void advanceProjectiles();
    void resolveProjectileCollisions();
    void handleShooting(common::ActionRequest action1, common::ActionRequest action2);
    void cleanupDestroyedEntities();
    void checkGameEndConditions(common::ActionRequest action1, common::ActionRequest action2);

    // Internal state:
    Board board_;
    std::size_t maxSteps_{0};
    std::size_t currentStep_{0};
    bool gameOver_{false};
    std::string resultStr_;

    // All tanks in birth order:
    std::vector<TankState> all_tanks_;
    // Map (player_index, tank_index) → index in all_tanks_
    std::vector<std::vector<std::size_t>> tankIdMap_;

    // All TankAlgorithm instances (in birth order) to ask getAction/updateBattleInfo:
    std::vector<std::unique_ptr<common::TankAlgorithm>> all_tank_algorithms_;

    // All Players (just two):
    std::unique_ptr<common::Player> player1_, player2_;
    std::unique_ptr<common::PlayerFactory> player_factory_;
    std::unique_ptr<common::TankAlgorithmFactory> tank_factory_;

    // All projectiles in flight:
    std::vector<std::unique_ptr<Projectile>> projectiles_;

    // Grid size & shell count:
    std::size_t rows_, cols_;
    std::size_t num_shells_;

    int nextTankIndex_[3]; // nextTankIndex_[1], nextTankIndex_[2]
};

} // namespace arena
