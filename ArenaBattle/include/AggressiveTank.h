// include/AggressiveTank.h
#pragma once

#include "common/TankAlgorithm.h"
#include "MyBattleInfo.h"

namespace arena {

/**
 * Aggressive: on first call to getAction() → GetBattleInfo;
 * thereafter, shoot anything directly ahead (if shellsLeft_>0),
 * otherwise MoveForward.
 */
class AggressiveTank : public common::TankAlgorithm {
public:
    AggressiveTank(int playerIndex, int tankIndex);
    ~AggressiveTank() override = default;

    void                  updateBattleInfo(common::BattleInfo &baseInfo) override;
    common::ActionRequest getAction() override;

private:
    MyBattleInfo          lastInfo_;       // last snapshot + self pos
    std::size_t           shellsLeft_ = SIZE_MAX; // init sentinel
    int                   direction_;      // 0..7, our own facing
};

} // namespace arena
