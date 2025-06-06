// In AggressiveTank.h:
#pragma once
#include "TankAlgorithm.h"
#include "MyBattleInfo.h"

class AggressiveTank : public TankAlgorithm {
public:
    AggressiveTank(int playerIndex, int tankIndex);
    ~AggressiveTank() override;

    // override the base‐class method
    void updateBattleInfo(const common::BattleInfo& info) override;
    common::ActionRequest getAction() override;

private:
    // store a reference/copy of the last‐received MyBattleInfo
    MyBattleInfo lastInfo_;

    // track shells ourselves (same as lastInfo_.shellsRemaining,
    // but we decrement on every Shoot)
    int shellsRemaining_ = 0;
};
