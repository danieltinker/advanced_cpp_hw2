#include "MyPlayerFactory.h"

namespace arena {

std::unique_ptr<common::Player> MyPlayerFactory::create(
    int player_index,
    size_t x, size_t y,
    size_t max_steps,
    size_t num_shells
) const {
    return std::make_unique<MyPlayer>(
        player_index, x, y, max_steps, num_shells
    );
}

} // namespace arena
