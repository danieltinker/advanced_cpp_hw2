#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <set>

#include "Board.h"
#include "MySatelliteView.h"
#include "common/Player.h"
#include "common/PlayerFactory.h"
#include "common/TankAlgorithm.h"
#include "common/TankAlgorithmFactory.h"
#include "common/ActionRequest.h"
#include "common/SatelliteView.h"

namespace arena {

/// Maintains the board, tanks, shells, and orchestrates one‐turn advances.
class GameState {
public:
    GameState(std::unique_ptr<common::PlayerFactory> pFac,
              std::unique_ptr<common::TankAlgorithmFactory> tFac);
    ~GameState();

    /// Populate from a parsed Board, maxSteps, and shells per tank.
    void initialize(const Board& board, std::size_t maxSteps, std::size_t numShells);

    /// Advance one tick: rotate, move, shoot, resolve, and return actions.
    std::string advanceOneTurn();

    bool        isGameOver()     const;
    std::string getResultString() const;

    /// Display current board to stdout.
    void printBoard() const;

private:
    // Helpers for each sub-step:
    void applyTankRotations(const std::vector<common::ActionRequest>& actions);
    void handleTankMineCollisions();
    void updateTankCooldowns();
    void confirmBackwardMoves(std::vector<bool>& ignored,
                              const std::vector<common::ActionRequest>& actions);
    void updateTankPositionsOnBoard(std::vector<bool>& ignored,
                                    std::vector<bool>& killedThisTurn,
                                    const std::vector<common::ActionRequest>& actions);
    void handleShooting(std::vector<bool>& ignored,
                        const std::vector<common::ActionRequest>& actions);
    void updateShellsWithOverrunCheck();
    void resolveShellCollisions();
    bool handleShellMidStepCollision(int x, int y);
    void cleanupDestroyedEntities();
    void checkGameEndConditions();

    std::unique_ptr<common::SatelliteView>
    createSatelliteViewFor(int queryX, int queryY) const;

    static const char* directionToArrow(int dir);

    // ---- Internal state ----
    Board  board_;
    std::size_t rows_{0}, cols_{0};
    std::size_t maxSteps_{0}, currentStep_{0};
    bool        gameOver_{false};
    std::string resultStr_;

    struct TankState {
        int           player_index;
        int           tank_index;
        int           x, y, direction;
        bool          alive;
        std::size_t   shells_left;
        bool          wantsBackward;
    };
    std::vector<TankState> all_tanks_;
    std::vector<std::vector<std::size_t>> tankIdMap_;

    std::vector<std::unique_ptr<common::TankAlgorithm>> all_tank_algorithms_;
    std::unique_ptr<common::Player> player1_, player2_;
    std::unique_ptr<common::PlayerFactory>        player_factory_;
    std::unique_ptr<common::TankAlgorithmFactory> tank_factory_;

    struct Shell { int x, y, dir; };
    std::vector<Shell> shells_;
    std::set<std::size_t> toRemove_;
    std::map<std::pair<int,int>, std::vector<std::size_t>> positionMap_;

    std::size_t num_shells_{0};
    int nextTankIndex_[3]{0,0,0};
};

} // namespace arena
