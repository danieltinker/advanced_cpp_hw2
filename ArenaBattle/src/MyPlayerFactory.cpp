#include "MyPlayerFactory.h"

namespace arena {

std::unique_ptr<common::Player> MyPlayerFactory::create(
    int player_index,
    std::size_t x,
    std::size_t y,
    std::size_t max_steps,
    std::size_t num_shells
) const {
    // x == rows, y == cols (passed in by GameManager), but
    // we also have rows_ and cols_ stored here. Typically you'd verify:
    //   if (x != rows_ || y != cols_) { /* handle mismatch */ }
    // For now, just forward into MyPlayer’s constructor:
    return std::make_unique<MyPlayer>(
        player_index,
        max_steps,
        num_shells,
        x,  // board rows
        y   // board cols
    );
}

} // namespace arena
