#pragma once

#include "common/TankAlgorithm.h"
#include "MyBattleInfo.h"
#include "ActionRequest.h"
#include <deque>

namespace arena {

class AggressiveTank : public common::TankAlgorithm {
public:
    AggressiveTank(int playerIndex, int tankIndex);
    void updateBattleInfo(common::BattleInfo& info) override;
    common::ActionRequest getAction() override;

private:
    MyBattleInfo                            lastInfo_;
    int                                      shellsLeft_{-1};
    char                                     enemyChar_;
    int                                      direction_;
    bool                                     seenInfo_{false};

    // algorithm‐level cooldown before next Shoot request
    int                                      algoShootCooldown_{0};
    static constexpr int                    ALG_SHOOT_CD = 4;

    std::deque<common::ActionRequest>        plan_;

    int  encode(int x,int y,int d) const;
    void decode(int code,int& x,int& y,int& d) const;
};

} // namespace arena
