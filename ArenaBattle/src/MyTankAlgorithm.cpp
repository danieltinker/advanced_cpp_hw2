#include "MyTankAlgorithm.h"

using namespace arena;

MyTankAlgorithm::MyTankAlgorithm(int player_index, int tank_index)
    : common::TankAlgorithm(player_index, tank_index)  // invoke base‐class ctor
{
    // Any additional initialization for your AI can go here.
}

common::ActionRequest MyTankAlgorithm::getAction() {
    // TODO: return next action, default DoNothing
    return common::ActionRequest::DoNothing;
}

void MyTankAlgorithm::updateBattleInfo(common::BattleInfo& info) {
    // TODO: cast `info` to your MyBattleInfo and update internal state
}
