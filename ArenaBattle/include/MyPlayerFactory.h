#pragma once

#include "common/PlayerFactory.h"
#include "MyPlayer.h"

#include <cstddef>
#include <memory>

namespace arena {

/*
  MyPlayerFactory remembers (rows,cols).  GameManager will call create(
    player_index, rows, cols, max_steps, num_shells ).
*/
class MyPlayerFactory : public common::PlayerFactory {
public:
    MyPlayerFactory(std::size_t rows, std::size_t cols)
        : rows_(rows), cols_(cols) {}

    // Now matches the five‐parameter signature:
    virtual std::unique_ptr<common::Player> create(
        int player_index,
        std::size_t x,
        std::size_t y,
        std::size_t max_steps,
        std::size_t num_shells
    ) const override;

private:
    std::size_t rows_, cols_;
};

} // namespace arena
