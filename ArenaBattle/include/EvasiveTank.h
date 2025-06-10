// include/EvasiveTank.h
#pragma once

#include "common/TankAlgorithm.h"
#include "MyBattleInfo.h"

namespace arena {

class EvasiveTank : public common::TankAlgorithm {
public:
    EvasiveTank(int playerIndex, int tankIndex);
    ~EvasiveTank() override = default;

    void updateBattleInfo(common::BattleInfo& baseInfo) override;
    common::ActionRequest getAction() override;

private:
    MyBattleInfo lastInfo_;
    int           shellsLeft_;
    int           direction_;

    // 8‐way deltas: 0=up,1=up‐right,…7=up‐left
    static constexpr int D8[8][2] = {
        { 0,-1}, {+1,-1}, {+1,0}, {+1,+1},
        { 0,+1}, {-1,+1}, {-1,0}, {-1,-1}
    };
};

} // namespace arena
