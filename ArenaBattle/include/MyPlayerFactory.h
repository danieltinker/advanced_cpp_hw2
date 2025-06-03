#pragma once

#include "common/PlayerFactory.h"
#include "MyPlayer.h"

#include <cstddef>
#include <memory>

namespace arena {

/*
  MyPlayerFactory simply remembers (rows,cols) so it can pass them into each MyPlayer.
  GameManager constructs this factory with knowledge of rows, cols.
*/
class MyPlayerFactory : public common::PlayerFactory {
public:
    // Just declare constructor here; define it in MyPlayerFactory.cpp
    MyPlayerFactory(std::size_t rows, std::size_t cols);

    // Match the signature in common/PlayerFactory.h, but drop 'override'
    std::unique_ptr<common::Player> create(
        int player_index,
        std::size_t x,
        std::size_t y,
        std::size_t max_steps,
        std::size_t num_shells
    ) const;

private:
    std::size_t rows_, cols_;
};

} // namespace arena
