#pragma once

namespace common {

class BattleInfo {
public:
    virtual ~BattleInfo() {}
    // Concrete subclasses (MyBattleInfo) will carry the 2D grid, etc.
};

} // namespace common
