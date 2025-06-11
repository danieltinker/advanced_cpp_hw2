// AggressiveTank.h
#pragma once

#include "common/TankAlgorithm.h"
#include "MyBattleInfo.h"
#include "common/ActionRequest.h"
#include <deque>

namespace arena {

class AggressiveTank : public common::TankAlgorithm {
public:
    AggressiveTank(int playerIndex, int /*tankIndex*/);
    void updateBattleInfo(common::BattleInfo& info) override;
    common::ActionRequest getAction() override;

private:
    MyBattleInfo                            lastInfo_;
    int                                     shellsLeft_{-1};
    bool                                    seenInfo_{false};
    int                                     algoCooldown_{0};
    std::deque<common::ActionRequest>       plan_;
    int                                     ticksSinceInfo_{0};
    static constexpr int                    REFRESH_INTERVAL = 5;

    int                                     curX_{0};
    int                                     curY_{0};
    int                                     curDir_{0};
    int                                     playerIndex_{0};   // track which side we are on
    
    static constexpr int                    ROTATE_COST = 1;
    static constexpr int                    MOVE_COST   = 1;
    static constexpr int                    SHOOT_CD    = 4;

    void computePlan();
    bool lineOfSight(int startX, int startY, int dir, int& distSteps, int& wallsHit) const;
    bool isTraversable(int x, int y) const;
};

} // namespace arena
