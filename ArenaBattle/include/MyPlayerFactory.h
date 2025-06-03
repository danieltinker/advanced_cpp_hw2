#pragma once

#include "PlayerFactory.h"
#include "MyPlayer.h"

#include <cstddef>
#include <memory>

/*
  MyPlayerFactory simply remembers (rows,cols) so it can pass them into each MyPlayer.
  GameManager constructs this factory with knowledge of rows, cols.
*/
class MyPlayerFactory : public common::PlayerFactory {
public:
    MyPlayerFactory(std::size_t rows, std::size_t cols)
        : rows_(rows), cols_(cols) {}

    virtual std::unique_ptr<common::Player> create(
        int player_index,
        std::size_t max_steps,
        std::size_t num_shells) const override
    {
        return std::make_unique<MyPlayer>(
            player_index, max_steps, num_shells, rows_, cols_);
    }

private:
    std::size_t rows_, cols_;
};
