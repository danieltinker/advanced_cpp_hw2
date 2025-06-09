// include/EvasiveTank.h
#pragma once

#include "common/TankAlgorithm.h"
#include "MyBattleInfo.h"

namespace arena {

class EvasiveTank : public common::TankAlgorithm {
public:
    EvasiveTank(int playerIndex, int tankIndex);
    void updateBattleInfo(common::BattleInfo& baseInfo) override;
    common::ActionRequest getAction() override;

private:
    MyBattleInfo lastInfo_;
    int          direction_;  // 0..7
    bool         needView_;   // always fetch fresh view each turn
    char         enemyChar_;  // '1' or '2'

    static constexpr int DX[8] = { 0, +1, +1, +1, 0, -1, -1, -1 };
    static constexpr int DY[8] = {-1, -1,  0, +1,+1, +1,  0, -1 };
};

} // namespace arena
