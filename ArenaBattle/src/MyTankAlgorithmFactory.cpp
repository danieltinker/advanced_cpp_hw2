#include "MyTankAlgorithmFactory.h"

namespace arena {

std::unique_ptr<common::TankAlgorithm>
MyTankAlgorithmFactory::create(int player_index, int tank_index) const {
    return std::make_unique<MyTankAlgorithm>(player_index, tank_index);
}

} // namespace arena
