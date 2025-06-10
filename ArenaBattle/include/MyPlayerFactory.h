#pragma once

#include <memory>
#include "common/PlayerFactory.h"
#include "common/Player.h"
#include "Player1.h"
#include "Player2.h"

namespace arena {

// Concrete PlayerFactory: default-constructible, and also template for rows/cols if needed.
class MyPlayerFactory : public common::PlayerFactory {
public:
    // Default ctor so you can do MyPlayerFactory{} in main
    MyPlayerFactory() = default;

    ~MyPlayerFactory() override = default;

    // Must match exactly common::PlayerFactory::create signature
    std::unique_ptr<common::Player>
    create(int player_index,
           std::size_t rows,
           std::size_t cols,
           std::size_t max_steps,
           std::size_t num_shells) const override
    {
        if (player_index == 1) {
            return std::make_unique<Player1>(
                player_index, rows, cols, max_steps, num_shells
            );
        } else {
            return std::make_unique<Player2>(
                player_index, rows, cols, max_steps, num_shells
            );
        }
    }

private:
    // no longer required; create() uses its parameters
    // std::size_t rows_ = 0;
    // std::size_t cols_ = 0;
};

} // namespace arena