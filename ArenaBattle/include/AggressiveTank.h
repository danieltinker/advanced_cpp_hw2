#pragma once

#include "common/TankAlgorithm.h"
#include "common/ActionRequest.h"
#include "MyBattleInfo.h"

#include <deque>
#include <vector>
#include <utility>

namespace arena {

class AggressiveTank : public common::TankAlgorithm {
public:
    AggressiveTank(int playerIndex, int /*tankIndex*/);

    // Called by the framework whenever fresh battle info arrives
    void updateBattleInfo(common::BattleInfo& info) override;

    // Do we need to issue a GetBattleInfo next?
    bool needsInfo();

    // Returns the next action to take
    common::ActionRequest getAction() override;

private:
    struct State { int x, y, dir; };
    // Added 'ds' to store distance (number of steps) to target
    struct ShootState { State st; int walls; int ds; };

    // Core planning routines
    void computePlan();
    std::vector<ShootState>            findShootStates() const;
    std::vector<common::ActionRequest> planPathTo(const State& target);
    std::vector<common::ActionRequest> buildShootSequence(int walls) const;
    std::vector<common::ActionRequest> fallbackRoam() const;

    // Line‐of‐sight and movement tests
    bool lineOfSight(int sx, int sy, int dir, int& ds, int& wh) const;
    bool isTraversable(int x, int y) const;

    // Last‐seen info snapshot
    MyBattleInfo                       lastInfo_;
    int                                playerIndex_;
    int                                curX_, curY_, curDir_;
    int                                shellsLeft_;
    bool                               seenInfo_;
    int                                ticksSinceInfo_;
    int                                algoCooldown_;

    // Current plan of actions (pop_front on each step)
    std::deque<common::ActionRequest>  plan_;

    // Tunable constants
    static constexpr int REFRESH_INTERVAL = 20;
    static constexpr int SHOOT_CD          = 5;
    static constexpr int MOVE_COST         = 1;
    static constexpr int ROTATE_COST       = 1;

    // Rotation delta → ActionRequest lookup (fixed size for range-based for)
    static const std::pair<int, common::ActionRequest> ROTATIONS[4];
};

} // namespace arena
