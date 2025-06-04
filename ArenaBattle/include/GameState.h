#pragma once

#include <string>
#include <vector>
#include <memory>

#include "Board.h"
#include "Projectile.h"
#include "Tank.h"

#include "MyBattleInfo.h"
#include "MySatelliteView.h"

#include "common/TankAlgorithm.h"
#include "common/Player.h"
#include "common/PlayerFactory.h"
#include "common/TankAlgorithmFactory.h"
#include "common/ActionRequest.h"
#include "common/SatelliteView.h"

namespace arena {

/*
  GameState now owns *all* of the state and tick logic:

    • Board board_  (walls '#', mines '@', tank chars '1'/'2')
    • a vector<TankState> all_tanks_ in birth order
    • a vector<unique_ptr<Projectile>> projectiles_
    • Two Player objects (player1_, player2_) that handle GetBattleInfo calls
    • A parallel vector of TankAlgorithm instances (one per tank)
    • maxSteps_, currentStep_, gameOver_, resultStr_
    • Rows/cols/num_shells_ and a two‐dimensional tankIdMap_ for lookups

  Public interface:
    - GameState(factory1, factory2)
    - initialize(board, maxSteps, numShells)
    - advanceOneTurn() → string describing exactly what every tank did
    - isGameOver() / getResultString()

  All of the old “move‐resolve‐shoot” code has been centralized here.
*/
class GameState {
public:
    // Constructs an empty GameState.  Must call initialize(...) before advanceOneTurn().
    GameState(std::unique_ptr<common::PlayerFactory> pFac,
              std::unique_ptr<common::TankAlgorithmFactory> tFac);
    ~GameState();

    /// Initialize from a parsed Board, maxSteps, and shells per tank.
    /// This will:
    ///   1) copy in `board`, store rows_/cols_, maxSteps_, num_shells_
    ///   2) scan the board for '1'/'2' to build all_tanks_ and tankIdMap_
    ///   3) create two Player instances via pFac->create(...)
    ///   4) create one TankAlgorithm per tank via tFac->create(...)
    ///   5) set currentStep_=0, gameOver_=false
    void initialize(const Board& board, std::size_t maxSteps, std::size_t numShells);

    /// Advance exactly one tick for *all* tanks:
    ///   • Collect each alive tank’s ActionRequest, handling any GetBattleInfo
    ///   • Perform the 10 sub‐phases (rotate, move, mines, cooldowns, projectiles, shoot, cleanup, end‐check)
    ///   • Return a comma‐separated string (“Action1, Action2 (ignored), …”) describing what happened
    std::string advanceOneTurn();

    bool isGameOver() const;
    std::string getResultString() const;

    /// (Optional) If some tank needs a raw SatelliteView, you can call this with that tank’s (x,y).
    std::unique_ptr<common::SatelliteView>
    createSatelliteViewFor(int queryX, int queryY) const;

private:
    struct TankState {
        int player_index;       // 1 or 2
        int tank_index;         // 0‐based within that player
        std::size_t x, y;       // current board coordinates
        int direction;          // 0..7 (0=Up,1=Up‐Right,…,6=Left,7=Up‐Left)
        bool alive;
        std::size_t shells_left;
        bool wantsBackward_;    // used by confirmBackwardMoves()
    };

    // ========== Private helpers for each sub‐step ==========

    // (A) Rotations: update direction for each tank that asked RotateLeft/RotateRight
    void applyTankRotations(const std::vector<common::ActionRequest>& actions);

    // (B) Handle any tank that moved onto a mine “@”
    void handleTankMineCollisions();

    // (C) Decrement any “cooldown” counters if implemented (currently no per‐tank cooldown logic)
    void updateTankCooldowns();

    // (D) Confirm that “MoveBackward” was legal; otherwise mark “ignored” later
    void confirmBackwardMoves(std::vector<bool>& ignored,
                              const std::vector<common::ActionRequest>& actions);

    // (E) Actually update each tank’s position in board_ (for forward/backward)
    void updateTankPositionsOnBoard(std::vector<bool>& ignored,
                                    std::vector<bool>& killedThisTurn,
                                    const std::vector<common::ActionRequest>& actions);

    // (F) Advance all projectiles by one cell
    void advanceProjectiles();

    // (G) Resolve any projectile collisions: wall, mine, tank
    void resolveProjectileCollisions(std::vector<bool>& killedThisTurn);

    // (H) Spawn new projectiles for each Shoot action
    void handleShooting(const std::vector<common::ActionRequest>& actions,
                        std::vector<bool>& killedThisTurn);

    // (I) Remove dead tanks and projectiles that are out‐of‐bounds or hit walls
    void cleanupDestroyedEntities();

    // (J) Check game‐over conditions (zero tanks, shell starvation, maxSteps)
    void checkGameEndConditions();

    // ========== Internal state ==========

    Board board_;
    std::size_t rows_{0}, cols_{0};
    std::size_t maxSteps_{0};
    std::size_t currentStep_{0};
    bool gameOver_{false};
    std::string resultStr_;

    // All tanks (in birth order).  TankState.alive == false means tank is destroyed.
    std::vector<TankState> all_tanks_;

    // tankIdMap_[player_index][tank_index] = index into all_tanks_ or SIZE_MAX
    std::vector<std::vector<std::size_t>> tankIdMap_;

    // One TankAlgorithm instance per TankState (same index as all_tanks_)
    std::vector<std::unique_ptr<common::TankAlgorithm>> all_tank_algorithms_;

    // Two Player instances (for player1_ and player2_)
    std::unique_ptr<common::Player> player1_, player2_;
    std::unique_ptr<common::PlayerFactory> player_factory_;
    std::unique_ptr<common::TankAlgorithmFactory> tank_factory_;

    // All projectiles currently in flight
    std::vector<std::unique_ptr<Projectile>> projectiles_;

    // Number of shells each tank started with
    std::size_t num_shells_{0};

    // Used to assign per‐player tank_index in birth order
    int nextTankIndex_[3]{0, 0, 0};
};

} // namespace arena
