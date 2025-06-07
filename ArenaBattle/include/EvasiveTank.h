// include/EvasiveTank.h
#pragma once

#include "common/TankAlgorithm.h"
#include "MyBattleInfo.h"

namespace arena {

/**
 * Evasive: on first call → GetBattleInfo;
 * thereafter, if a shell (‘*’) is adjacent, MoveBackward;
 * if a wall or mine is directly ahead, RotateLeft90;
 * otherwise MoveForward.
 */
class EvasiveTank : public common::TankAlgorithm {
public:
    EvasiveTank(int playerIndex, int tankIndex);
    ~EvasiveTank() override = default;

    void                  updateBattleInfo(common::BattleInfo &baseInfo) override;
    common::ActionRequest getAction() override;

private:
    MyBattleInfo          lastInfo_;       // last snapshot + self pos
    std::size_t           shellsLeft_ = SIZE_MAX; // sentinel
    int                   direction_;      // 0..7
};

} // namespace arena
