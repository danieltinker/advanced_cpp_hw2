// AggressiveTank.cpp
#include "AggressiveTank.h"
#include <queue>
#include <algorithm>
#include <cmath>
#include <limits>

namespace arena {

AggressiveTank::AggressiveTank(int playerIndex, int /*tankIndex*/) noexcept
    : common::TankAlgorithm()
    , lastInfo_(0,0)
    , ammo_(0)
    , enemyId_(playerIndex == 1 ? '2' : '1')
    , dir_(playerIndex == 1 ? 6 : 2)  // player1 faces left (6), player2 faces right (2)
    , infoFetched_(false)
    , turnsSinceFetch_(kRefreshTurns)
{}

void AggressiveTank::updateBattleInfo(common::BattleInfo& info) {
    auto &mi = static_cast<MyBattleInfo&>(info);
    lastInfo_        = mi;
    ammo_            = static_cast<int>(mi.shellsRemaining);
    turnsSinceFetch_ = 0;
    plan_.clear();
    if (!infoFetched_) {
        // Mark that initial info has been fetched
        infoFetched_ = true;
    }
}

common::ActionRequest AggressiveTank::getAction() {
    if (!infoFetched_) return common::ActionRequest::GetBattleInfo;
    if (++turnsSinceFetch_ >= kRefreshTurns) {
        infoFetched_ = false;
        return common::ActionRequest::GetBattleInfo;
    }
    if (!plan_.empty()) {
        auto act = plan_.front(); plan_.pop_front();
        return act;
    }

    int sx = static_cast<int>(lastInfo_.selfX);
    int sy = static_cast<int>(lastInfo_.selfY);
    auto [tx, ty] = findNearestEnemy(sx, sy);

    // SHOOT if clear line-of-sight to an enemy ('1' or '2'), not '%'
    if (ammo_ > 0 && hasLineOfSight(sx, sy, tx, ty)) {
        plan_.push_back(common::ActionRequest::Shoot);
        --ammo_;
    } else {
        // Movement: A* path or fallback
        auto path = findPath(sx, sy, tx, ty);
        if (path.size() > 1) {
            auto [nx, ny] = path[1];
            int desired = chooseDirection(nx - sx, ny - sy);
            if (desired != dir_) plan_.push_back(makeRotate(desired));
            plan_.push_back(common::ActionRequest::MoveForward);
        } else {
            int dx = tx - sx, dy = ty - sy;
            int desired = chooseDirection(dx, dy);
            if (desired != dir_) plan_.push_back(makeRotate(desired));
            int fx = sx + kDirOffsets[desired].first;
            int fy = sy + kDirOffsets[desired].second;
            if (inBounds(fx, fy) && lastInfo_.grid[fy][fx] == '.') {
                plan_.push_back(common::ActionRequest::MoveForward);
            }
        }
    }

    return plan_.empty()
         ? common::ActionRequest::GetBattleInfo
         : plan_.front();
}

bool AggressiveTank::hasLineOfSight(int x0, int y0, int x1, int y1) const noexcept {
    int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    while (true) {
        if (x0 == x1 && y0 == y1) return true;
        if (!inBounds(x0, y0) || lastInfo_.grid[y0][x0] == '#') return false;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

int AggressiveTank::chooseDirection(int dx, int dy) const noexcept {
    int best = dir_, maxDot = std::numeric_limits<int>::min();
    for (int d = 0; d < 8; ++d) {
        auto [ox, oy] = kDirOffsets[d];
        int dot = ox * dx + oy * dy;
        if (dot > maxDot) { maxDot = dot; best = d; }
    }
    return best;
}

std::pair<int,int> AggressiveTank::findNearestEnemy(int sx, int sy) const noexcept {
    int bestDist = std::numeric_limits<int>::max();
    std::pair<int,int> target{sx, sy};
    for (int y = 0; y < static_cast<int>(lastInfo_.rows); ++y) {
        for (int x = 0; x < static_cast<int>(lastInfo_.cols); ++x) {
            char c = lastInfo_.grid[y][x];
            if ((enemyId_ == '1' && c == '1') || (enemyId_ == '2' && c == '2')) {
                int d = std::abs(x - sx) + std::abs(y - sy);
                if (d < bestDist) { bestDist = d; target = {x, y}; }
            }
        }
    }
    return target;
}

std::vector<std::pair<int,int>> AggressiveTank::findPath(int sx, int sy, int tx, int ty) const {
    int W = static_cast<int>(lastInfo_.cols), H = static_cast<int>(lastInfo_.rows);
    auto idx = [&](int x, int y, int d) { return (y * W + x) * 8 + d; };
    int total = W * H * 8;

    struct Node { int cost, v; };
    auto cmp = [](Node const &a, Node const &b) { return a.cost > b.cost; };
    std::priority_queue<Node, std::vector<Node>, decltype(cmp)> pq(cmp);

    std::vector<int> dist(total, std::numeric_limits<int>::max());
    std::vector<int> prev(total, -1);
    std::vector<common::ActionRequest> via(total);

    for (int d = 0; d < 8; ++d) {
        int v = idx(sx, sy, d);
        dist[v] = 0;
        pq.push({0, v});
    }
    int goalV = -1;

    while (!pq.empty()) {
        auto [c, u] = pq.top(); pq.pop();
        if (c != dist[u]) continue;
        int d = u % 8;
        int x = (u / 8) % W;
        int y = (u / 8) / W;
        if (x == tx && y == ty) { goalV = u; break; }
        // Rotate neighbors
        for (auto [delta, action] : std::array<std::pair<int, common::ActionRequest>, 4>{{
            {-1, common::ActionRequest::RotateLeft45},
            { 1, common::ActionRequest::RotateRight45},
            {-2, common::ActionRequest::RotateLeft90},
            { 2, common::ActionRequest::RotateRight90}
        }}) {
            int nd = (d + delta + 8) % 8;
            int v2 = idx(x, y, nd);
            int w = c + (std::abs(delta) == 2 ? kRotateCost90 : kRotateCost45);
            if (w < dist[v2]) { dist[v2] = w; prev[v2] = u; via[v2] = action; pq.push({w, v2}); }
        }
        // Forward neighbor
        auto [ox, oy] = kDirOffsets[d];
        int nx = x + ox, ny = y + oy;
        if (inBounds(nx, ny) && lastInfo_.grid[ny][nx] == '.') {
            int v2 = idx(nx, ny, d);
            int w = c + kMoveCost;
            if (w < dist[v2]) { dist[v2] = w; prev[v2] = u; via[v2] = common::ActionRequest::MoveForward; pq.push({w, v2}); }
        }
    }

    std::vector<std::pair<int,int>> path;
    if (goalV >= 0) {
        int v = goalV;
        std::vector<common::ActionRequest> seq;
        while (prev[v] >= 0) { seq.push_back(via[v]); v = prev[v]; }
        std::reverse(seq.begin(), seq.end());
        int cx = sx, cy = sy;
        for (auto &a : seq) {
            if (a == common::ActionRequest::MoveForward) {
                auto [dx, dy] = kDirOffsets[v % 8];
                cx += dx; cy += dy;
                path.emplace_back(cx, cy);
                break;
            }
        }
    }
    if (path.empty()) path.emplace_back(sx, sy);
    return path;
}

common::ActionRequest AggressiveTank::makeRotate(int desiredDir) noexcept {
    int diff = (desiredDir - dir_ + 8) % 8;
    if (diff == 2) { dir_ = (dir_ + 2) % 8; return common::ActionRequest::RotateRight90; }
    if (diff == 6) { dir_ = (dir_ + 6) % 8; return common::ActionRequest::RotateLeft90; }
    if (diff <= 4) { dir_ = (dir_ + 1) % 8; return common::ActionRequest::RotateRight45; }
    dir_ = (dir_ + 7) % 8; return common::ActionRequest::RotateLeft45;
}

} // namespace arena
