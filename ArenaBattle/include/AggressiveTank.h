#pragma once

#include "common/TankAlgorithm.h"
#include "MyTankAlgorithm.h"
#include "MyBattleInfo.h"

namespace arena {

/**
 * An aggressive strategy: shoot any enemy directly in front (if we have shells),
 * otherwise move forward.  Internally tracks shellsRemaining_ each turn.
 */
class AggressiveTank : public MyTankAlgorithm{
public:
    AggressiveTank(int playerIndex, int tankIndex);
    ~AggressiveTank() override;

    void updateBattleInfo(common::BattleInfo &baseInfo) override;
    common::ActionRequest getAction() override;

private:
    MyBattleInfo lastInfo_;   ///< The most recent snapshot of the board and our own state
    int          shellsRemaining_ = 0;
};

} // namespace arena
