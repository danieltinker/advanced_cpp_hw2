#include "MyPlayerFactory.h"

namespace arena {

MyPlayerFactory::MyPlayerFactory(std::size_t rows, std::size_t cols)
    : rows_(rows), cols_(cols)
{}

std::unique_ptr<common::Player> MyPlayerFactory::create(
    int player_index,
    std::size_t x,
    std::size_t y,
    std::size_t max_steps,
    std::size_t num_shells
) const {
    // (x,y) parameters are just placeholders; we use stored rows_, cols_
    return std::make_unique<MyPlayer>(
        player_index,
        max_steps,
        num_shells,
        rows_,
        cols_
    );
}

} // namespace arena
