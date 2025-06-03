#pragma once

#include "TankAlgorithm.h"
#include "MyBattleInfo.h"

#include <cstddef>

class MyTankAlgorithm : public common::TankAlgorithm {
public:
    MyTankAlgorithm(int player_index, int tank_index, std::size_t num_shells)
        : player_index_(player_index),
          tank_index_(tank_index),
          direction_((player_index == 1) ? 6 : 2),
          shells_left_(num_shells),
          pos_x_(SIZE_MAX),
          pos_y_(SIZE_MAX),
          has_new_info_(false)
    {}

    virtual common::ActionRequest getAction() override {
        if (!has_new_info_) {
            has_new_info_ = true;
            return common::ActionRequest::GetBattleInfo;
        }
        has_new_info_ = false;
        return common::ActionRequest::RotateRight90;
    }

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

    std::size_t getX() const { return pos_x_; }
    std::size_t getY() const { return pos_y_; }
    int getDirection() const { return direction_; }
    std::size_t getShellsLeft() const { return shells_left_; }
    int getPlayerIndex() const { return player_index_; }
    int getTankIndex() const { return tank_index_; }

private:
    int player_index_;
    int tank_index_;
    int direction_;
    std::size_t shells_left_;
    std::size_t pos_x_, pos_y_;
    bool has_new_info_;
};
