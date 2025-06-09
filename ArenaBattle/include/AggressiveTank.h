#pragma once
#include "common/TankAlgorithm.h"
#include "MyBattleInfo.h"
#include <deque>

namespace arena {

class AggressiveTank : public common::TankAlgorithm {
public:
    AggressiveTank(int playerIndex, int tankIndex);
    void updateBattleInfo(common::BattleInfo& baseInfo) override;
    common::ActionRequest getAction() override;

private:
    MyBattleInfo                         lastInfo_;
    int                                  shellsLeft_;
    char                                 enemyChar_;
    int                                  direction_;
    int                                  viewCooldown_;   // turns until next GetBattleInfo
    bool                                 justShot_;       // force view after a shot
    std::deque<common::ActionRequest>    actionQueue_;

    static constexpr int DX[8] = { 0, +1, +1, +1, 0, -1, -1, -1 };
    static constexpr int DY[8] = {-1, -1,  0, +1,+1, +1,  0, -1 };
};

} // namespace arena
