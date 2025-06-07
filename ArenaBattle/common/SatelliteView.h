#pragma once

#include <cstddef>

namespace common {

/*
  GameManager builds a concrete MySatelliteView and passes
  it to Player when a tank requests GetBattleInfo. A SatelliteView
  can be queried at (x,y), returning any of: '#','1','2','%','@','*',' ','&'.
*/
class SatelliteView {
public:
    virtual ~SatelliteView() {}
    virtual char getObjectAt(std::size_t x, std::size_t y) const = 0;
};

} // namespace common
