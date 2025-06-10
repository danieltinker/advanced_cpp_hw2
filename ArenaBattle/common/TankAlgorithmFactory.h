#pragma once
#include "TankAlgorithm.h"

namespace common {

class TankAlgorithmFactory {
public:
    virtual ~TankAlgorithmFactory() {}

    // When GameManager spawns each tank, it calls:
    //   tankFactory->create(player_index, tank_index)
    virtual std::unique_ptr<TankAlgorithm> create(
        int player_index,
        int tank_index) const = 0;
};

} // namespace common
