#pragma once

#include <memory>
#include "common/TankAlgorithmFactory.h"
#include "AggressiveTank.h"
#include "EvasiveTank.h"

namespace common {

// Concrete TankAlgorithmFactory: default-constructible, and also accepts num_shells if main does
class MyTankAlgorithmFactory : public TankAlgorithmFactory {
public:
    // Default ctor
    MyTankAlgorithmFactory() = default;

    ~MyTankAlgorithmFactory() override = default;

    // Must match exactly common::TankAlgorithmFactory::create signature
    std::unique_ptr<TankAlgorithm>
    create(int player_index, int tank_index) const override
    {
        if (player_index == 1) {
            return std::make_unique<arena::AggressiveTank>(
                player_index, tank_index
            );
        } else {
            return std::make_unique<arena::EvasiveTank>(
                player_index, tank_index
            );
        }
    }

};

} // namespace common