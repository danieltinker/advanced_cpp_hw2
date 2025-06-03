#pragma once
#include <memory>
#include "Player.h"

namespace common {

class PlayerFactory {
public:
    virtual ~PlayerFactory() {}

    // Given (player_index ∈ {1,2}, max_steps, num_shells), produce a new Player
    virtual std::unique_ptr<Player> create(
        int player_index,
        std::size_t max_steps,
        std::size_t num_shells) const = 0;
};

} // namespace common
