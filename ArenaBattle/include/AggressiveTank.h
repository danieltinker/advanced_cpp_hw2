// AggressiveTank.h
#ifndef AGGRESSIVETANK_H
#define AGGRESSIVETANK_H

#include "common/BattleInfo.h"
#include "common/ActionRequest.h"
#include "common/TankAlgorithm.h"
#include "MyBattleInfo.h"  // Extended info with selfX, selfY, shellsRemaining, grid
#include <deque>
#include <vector>

namespace arena {

class AggressiveTank : public common::TankAlgorithm {
public:
    AggressiveTank(int playerIndex, int tankIndex);
    void updateBattleInfo(common::BattleInfo& info) override;
    common::ActionRequest getAction() override;

private:
    // Timing & cost constants
    static constexpr int REFRESH_INTERVAL = 5;
    static constexpr int SHOOT_CD         = 2;
    static constexpr int ROTATE_COST      = 1;
    static constexpr int MOVE_COST        = 2;

    struct State { int x, y, dir; };
    struct ShootState { State st; int walls; };

    bool needsInfo();
    void computePlan();
    std::vector<ShootState> findShootStates() const;
    std::vector<common::ActionRequest> planPathTo(const State& target);
    std::vector<common::ActionRequest> buildShootSequence(int walls) const;
    std::vector<common::ActionRequest> fallbackRoam() const;
    bool lineOfSight(int sx, int sy, int dir, int& ds, int& wh) const;
    bool isTraversable(int x, int y) const;

    MyBattleInfo lastInfo_;
    int playerIndex_;
    int curX_, curY_, curDir_;
    int shellsLeft_;
    bool seenInfo_;
    int ticksSinceInfo_;
    int algoCooldown_;
    std::deque<common::ActionRequest> plan_;
};

} // namespace arena

#endif // AGGRESSIVETANK_H
