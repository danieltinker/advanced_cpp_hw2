// src/EvasiveTank.h
#pragma once

#include "common/TankAlgorithm.h"
#include "MyBattleInfo.h"

namespace arena {

class EvasiveTank : public common::TankAlgorithm {
public:
    EvasiveTank(int playerIndex, int tankIndex);
    ~EvasiveTank() override;

    void updateBattleInfo(common::BattleInfo& baseInfo) override;
    common::ActionRequest getAction() override;

private:
    MyBattleInfo lastInfo_;
    int shellsRemaining_ = 0;
};

} // namespace arena
