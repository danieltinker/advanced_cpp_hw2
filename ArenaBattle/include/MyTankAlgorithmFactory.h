#pragma once

#include "common/TankAlgorithmFactory.h"
#include "MyTankAlgorithm.h"

#include <cstddef>
#include <memory>

/*
  MyTankAlgorithmFactory just remembers num_shells so it can create each MyTankAlgorithm.
  GameManager constructs this with the correct num_shells.
*/
class MyTankAlgorithmFactory : public common::TankAlgorithmFactory {
public:
    MyTankAlgorithmFactory(std::size_t num_shells)
        : num_shells_(num_shells) {}

    virtual std::unique_ptr<common::TankAlgorithm> create(
        int player_index,
        int tank_index) const override
    {
        return std::make_unique<MyTankAlgorithm>(
            player_index, tank_index, num_shells_);
    }

private:
    std::size_t num_shells_;
};
