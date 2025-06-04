#include "MyTankAlgorithmFactory.h"

namespace arena {

std::unique_ptr<common::TankAlgorithm> MyTankAlgorithmFactory::create(
    int player_index,
    int tank_index) const
{
    // Pass num_shells_ as the third argument:
    return std::make_unique<MyTankAlgorithm>(
        player_index,
        tank_index,
        num_shells_   // ← include this!
    );
}

} // namespace arena
