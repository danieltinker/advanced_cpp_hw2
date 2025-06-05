#pragma once

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <set>

#include "Board.h"
#include "Tank.h"

#include "MySatelliteView.h"            // for building snapshots

#include "common/TankAlgorithm.h"
#include "common/Player.h"
#include "common/PlayerFactory.h"
#include "common/TankAlgorithmFactory.h"
#include "common/ActionRequest.h"
#include "common/SatelliteView.h"

namespace arena {

/*
  GameState implements HW1’s logic:

    • Each shell moves two cells per tick (with wraparound).
    • If a shell’s sub‐step hits a wall, increment wallHits; wall breaks on 2 hits.
    • If a shell hits a tank mid‐step, that tank dies immediately; shell removed.
    • If two or more shells end on the same cell, they destroy each other.
    • Tanks do not wrap. Forward/backward into a wall or OOB = ignored.
    • Stepping onto a mine kills the tank and removes the mine.
    • “GetBattleInfo” → build MySatelliteView with ‘%’ at that tank’s (x,y).
    • Walls print ‘#’, mines ‘@’, empty ‘ ’, shell overlay ‘*’, tank arrows in color.

  All single‐step “projectiles_” code has been removed in favor of the two‐step
  Shell logic.
*/

class GameState {
public:
    GameState(std::unique_ptr<common::PlayerFactory> pFac,
              std::unique_ptr<common::TankAlgorithmFactory> tFac);
    ~GameState();

    /// Initialize from parsed Board, maxSteps, and shells per tank.
    void initialize(const Board& board, std::size_t maxSteps, std::size_t numShells);

    /// Advance one tick for all tanks; return comma‐separated action string.
    std::string advanceOneTurn();

    bool isGameOver() const;
    std::string getResultString() const;

    /// Build a MySatelliteView (with ‘%’ at (queryX,queryY)) for a single tank.
    std::unique_ptr<common::SatelliteView> createSatelliteViewFor(int queryX, int queryY) const;

    /// Print the current board (walls '#', mines '@', empties ' '), overlay:
    ///  • shells as '*'
    ///  • tanks as colored arrows (red for Player1, blue for Player2)
    void printBoard() const;

private:
    // A single shell (missile) on the board:
    struct Shell {
        int x, y;       // current coords
        int dir;        // 0..7
    };

    // Per‐tank state (one per tank in birth order):
    struct TankState {
        int player_index;  // 1 or 2
        int tank_index;    // 0‐based within that player
        int x, y;          // board coords
        int direction;     // 0..7
        bool alive;
        std::size_t shells_left;
        bool wantsBackward_;
    };

    // (A) Rotations (changes direction if rotate request)
    void applyTankRotations(const std::vector<common::ActionRequest>& actions);

    // (B) If any tank stands on a mine '@', kill it
    void handleTankMineCollisions();

    // (C) Cooldowns (unused stub)
    void updateTankCooldowns();

    // (D) Confirm backward‐move legality; mark ignored[k]=true if illegal
    void confirmBackwardMoves(std::vector<bool>& ignored,
                              const std::vector<common::ActionRequest>& actions);

    // (E) Move tanks forward/backward (no wrap). Walls or OOB => ignored; mine => kill.
    void updateTankPositionsOnBoard(std::vector<bool>& ignored,
                                    std::vector<bool>& killedThisTurn,
                                    const std::vector<common::ActionRequest>& actions);

    // (F) Handle shooting: spawn new Shells at (x+dx,y+dy) with immediate mid‐step check
    void handleShooting(std::vector<bool>& ignored,const std::vector<common::ActionRequest>& actions
                        );

    // (G) Move all shells two sub‐steps each (wrap + mid‐step collisions)
    void updateShellsWithOverrunCheck();

    // (H) Resolve collisions between shells / walls / tanks after movement
    void resolveShellCollisions();

    // (I) Mid‐step collision check at (x,y): wall, tank, mine logic
    bool handleShellMidStepCollision(int x, int y);

    // (J) Cleanup: remove any shells flagged for deletion (already flagged in steps 7–9)
    void cleanupDestroyedEntities();

    // (K) Check if game‐over conditions are met (win/tie)
    void checkGameEndConditions();

    // Map a direction (0..7) to a UTF‐8 arrow string
    static const char* directionToArrow(int dir);

private:
    Board board_;
    std::size_t rows_{0}, cols_{0};
    std::size_t maxSteps_{0};
    std::size_t currentStep_{0};
    bool gameOver_{false};
    std::string resultStr_;

    std::vector<TankState> all_tanks_;
    // (playerIdx → vector[tankIndex] = index in all_tanks_)
    std::vector<std::vector<std::size_t>> tankIdMap_;

    // One TankAlgorithm per tank
    std::vector<std::unique_ptr<common::TankAlgorithm>> all_tank_algorithms_;

    // Two Player objects for updating battle info
    std::unique_ptr<common::Player> player1_, player2_;
    std::unique_ptr<common::PlayerFactory> player_factory_;
    std::unique_ptr<common::TankAlgorithmFactory> tank_factory_;

    // All shells currently in flight
    std::vector<Shell> shells_;
    // Bookkeeping: which shell indices to delete
    std::set<std::size_t> toRemove_;
    // Map (x,y) → list of shell indices that ended at (x,y)
    std::map<std::pair<int,int>, std::vector<std::size_t>> positionMap_;

    std::size_t num_shells_{0};
    int nextTankIndex_[3]{0, 0, 0};  // next index for player 1 and 2
};

} // namespace arena
