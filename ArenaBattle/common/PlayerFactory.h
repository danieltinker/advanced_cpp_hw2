// common/PlayerFactory.h
#pragma once

#include <memory>
#include "Player.h"       // common::Player
#include "Player1.h"      // should resolve via your INCLUDE paths
#include "Player2.h"

namespace common {
class PlayerFactory {
public:
	virtual ~PlayerFactory() {}
virtual std::unique_ptr<Player> create(int player_index, size_t x, size_t y,
                size_t max_steps, size_t num_shells ) const = 0;
};



} // namespace common
