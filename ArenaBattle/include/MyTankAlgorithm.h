#pragma once

#include "common/TankAlgorithm.h"
#include "MyBattleInfo.h"

#include <cstddef>

namespace arena {

/*
  A simple TankAlgorithm that:
    1. On the first getAction(), returns GetBattleInfo.
    2. After updateBattleInfo(), always returns RotateRight90.
*/
class MyTankAlgorithm : public common::TankAlgorithm {
public:
    MyTankAlgorithm(int player_index, int tank_index)
        : player_index_(player_index),
          tank_index_(tank_index),
          direction_((player_index == 1) ? 6 : 2),
          shells_left_(0),
          pos_x_(SIZE_MAX),
          pos_y_(SIZE_MAX),
          has_new_info_(false)
    {}

    // First call returns GetBattleInfo; second call returns RotateRight90
    virtual common::ActionRequest getAction() override {
        if (!has_new_info_) {
            has_new_info_ = true;
            return common::ActionRequest::GetBattleInfo;
        }
        has_new_info_ = false;
        return common::ActionRequest::Shoot;
    }

    // Receives a MyBattleInfo; extracts the '%' position
    virtual void updateBattleInfo(common::BattleInfo& rawInfo) override {
        MyBattleInfo& info = static_cast<MyBattleInfo&>(rawInfo);
        bool found = false;
        for (std::size_t i = 0; i < info.rows && !found; ++i) {
            for (std::size_t j = 0; j < info.cols; ++j) {
                if (info.grid[i][j] == '%') {
                    pos_x_ = i;
                    pos_y_ = j;
                    found = true;
                    break;
                }
            }
        }
        has_new_info_ = true;
    }

    // Accessors for testing/debugging (not strictly required)
    std::size_t getX() const { return pos_x_; }
    std::size_t getY() const { return pos_y_; }
    int getDirection() const { return direction_; }
    std::size_t getShellsLeft() const { return shells_left_; }
    int getPlayerIndex() const { return player_index_; }
    int getTankIndex() const { return tank_index_; }

private:
    int player_index_;
    int tank_index_;
    int direction_;            // 0..7
    std::size_t shells_left_;
    std::size_t pos_x_, pos_y_;
    bool has_new_info_;
};

} // namespace arena
