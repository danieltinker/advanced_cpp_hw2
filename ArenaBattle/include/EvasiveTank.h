// include/EvasiveTank.h
#pragma once

#include "common/TankAlgorithm.h"
#include "MyBattleInfo.h"

namespace arena {

/**
 * EvasiveTank: stays out of shell paths and avoids obstacles.
 * − pulls a fresh view every other turn (needView_).
 * − first turn always GetBattleInfo to learn ammo.
 * − scans for shells (‘*’) up to two steps away in all 8 dirs.
 * − treats walls (‘#’), mines (‘@’) and other tanks (‘1’/‘2’) as obstacles.
 * − picks the safest escape direction (farthest from nearest shell).
 */
class EvasiveTank : public common::TankAlgorithm {
public:
    EvasiveTank(int playerIndex, int tankIndex);
    ~EvasiveTank() override = default;

    void updateBattleInfo(common::BattleInfo& baseInfo) override;
    common::ActionRequest getAction() override;

private:
    MyBattleInfo   lastInfo_;
    int            direction_;    // 0..7
    int            shellsLeft_;   // sentinel = –1 until learned
    bool           needView_;     // toggle view/no-view per turn

    static constexpr int DX[8] = { 0, +1, +1, +1,  0, -1, -1, -1 };
    static constexpr int DY[8] = { -1,-1,  0, +1, +1, +1,  0, -1 };

    // Check if cell (x,y) is free (in-bounds, not wall/mine/tank)
    bool isFree(int x, int y) const;
};

} // namespace arena
