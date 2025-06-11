// AggressiveTank.h
#pragma once

#include "common/TankAlgorithm.h"
#include "MyBattleInfo.h"
#include "ActionRequest.h"
#include <deque>
#include <vector>
#include <array>
#include <utility>

namespace arena {

// Production-grade aggressive tank implementation.
// Fetches grid sparsely (every kRefreshTurns), uses A*/fallback for movement,
// and handles shooting with cooldown.
class AggressiveTank final : public common::TankAlgorithm {
public:
    AggressiveTank(int playerIndex, int tankIndex) noexcept;
    void updateBattleInfo(common::BattleInfo& info) override;
    common::ActionRequest getAction() override;

private:
    MyBattleInfo            lastInfo_;
    int                     ammo_{0};
    char                    enemyId_{'0'};
    int                     dir_{0};           // 0..7 current facing
    bool                    infoFetched_{false};
    int                     turnsSinceFetch_{0};

    std::deque<common::ActionRequest> plan_;

    static inline constexpr std::array<std::pair<int,int>,8> kDirOffsets = {{
        {0,-1}, {1,-1}, {1,0}, {1,1}, {0,1}, {-1,1}, {-1,0}, {-1,-1}
    }};
    static constexpr int    kRefreshTurns  = 4;
    static constexpr int    kRotateCost45  = 1;
    static constexpr int    kRotateCost90  = 2;
    static constexpr int    kMoveCost      = 1;

    bool inBounds(int x, int y) const noexcept {
        return x >= 0 && y >= 0
            && x < static_cast<int>(lastInfo_.cols)
            && y < static_cast<int>(lastInfo_.rows);
    }

    bool hasLineOfSight(int x0, int y0, int x1, int y1) const noexcept;
    int  chooseDirection(int dx, int dy) const noexcept;
    std::pair<int,int> findNearestEnemy(int sx, int sy) const noexcept;
    std::vector<std::pair<int,int>> findPath(int sx, int sy, int tx, int ty) const;
    common::ActionRequest makeRotate(int desiredDir) noexcept;
};

} // namespace arena