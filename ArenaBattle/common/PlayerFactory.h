#pragma once

#include <memory>
#include <cstddef>
#include "Player.h"

namespace common {

class PlayerFactory {
public:
    virtual ~PlayerFactory() {}

    // Now includes (x, y) as the board’s dimensions, plus max_steps and num_shells.
    virtual std::unique_ptr<Player> create(
        int player_index,
        std::size_t x,
        std::size_t y,
        std::size_t max_steps,
        std::size_t num_shells
    ) const = 0;
};

} // namespace common
