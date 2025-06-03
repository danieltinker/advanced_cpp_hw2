#pragma once

#include "Player.h"
#include "MyBattleInfo.h"

#include <vector>
#include <cstddef>

/*
  MyPlayer’s only job is: when GameManager sees a tank return GetBattleInfo,
  it calls:
      player->updateTankWithBattleInfo(*tankAlg, satelliteView);
  The Player must build a MyBattleInfo from the SatelliteView, then call
      tankAlg->updateBattleInfo(myBattInfo);
*/
class MyPlayer : public common::Player {
public:
    MyPlayer(int player_index, std::size_t max_steps, std::size_t num_shells,
             std::size_t rows, std::size_t cols)
        : common::Player(player_index, max_steps, num_shells),
          rows_(rows), cols_(cols)
    {}

    virtual void updateTankWithBattleInfo(
        common::TankAlgorithm& tankAlg,
        common::SatelliteView& satellite_view) override
    {
        MyBattleInfo info;
        info.rows = rows_;
        info.cols = cols_;
        info.grid.assign(rows_, std::vector<char>(cols_, ' '));
        for (std::size_t i = 0; i < rows_; ++i) {
            for (std::size_t j = 0; j < cols_; ++j) {
                info.grid[i][j] = satellite_view.getObjectAt(i, j);
            }
        }
        tankAlg.updateBattleInfo(info);
    }

private:
    std::size_t rows_, cols_;
};
