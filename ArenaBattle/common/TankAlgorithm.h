#pragma once

#include "ActionRequest.h"
#include "BattleInfo.h"

namespace common {

/*
  A TankAlgorithm must implement:
   - getAction(): returns one of the enum values.
       If it returns GetBattleInfo, GM pauses and calls updateBattleInfo().
   - updateBattleInfo(BattleInfo&): after a GetBattleInfo request, Player passes
       a derived BattleInfo, and the tank uses it to update its internal state.
*/
class TankAlgorithm {
public:
    virtual ~TankAlgorithm() {}
    virtual ActionRequest getAction() = 0;
    virtual void updateBattleInfo(BattleInfo& info) = 0;
};

} // namespace common
