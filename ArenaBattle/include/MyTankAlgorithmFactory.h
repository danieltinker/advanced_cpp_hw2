#pragma once

#include "common/TankAlgorithmFactory.h"
#include "MyTankAlgorithm.h"

#include <cstddef>
#include <memory>

namespace arena {

/*
  MyTankAlgorithmFactory just remembers num_shells so it can create each MyTankAlgorithm.
  GameManager constructs this factory with the correct num_shells.
*/
class MyTankAlgorithmFactory : public common::TankAlgorithmFactory {
public:
    MyTankAlgorithmFactory(std::size_t num_shells)
        : num_shells_(num_shells) {}

    // Declaration only; definition will be in the .cpp
    std::unique_ptr<common::TankAlgorithm> create(
        int player_index,
        int tank_index) const override;

private:
    std::size_t num_shells_;
};

} // namespace arena
