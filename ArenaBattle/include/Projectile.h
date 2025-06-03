#pragma once

#include <cstddef>

/*
 A Projectile (shell) travels from one cell in a straight direction.
 For simplicity, a Projectile only needs its current (x,y) and direction.
*/
class Projectile {
public:
    Projectile(std::size_t x, std::size_t y, int dir)
        : x_(x), y_(y), direction_(dir)
    {}

    std::size_t getX() const { return x_; }
    std::size_t getY() const { return y_; }
    int getDirection() const { return direction_; }

    // Move one step along direction_ (0=Up,1=Up-Right,...,6=Left,7=Up-Left)
    void advance() {
        switch (direction_) {
            case 0:  x_--; break; // Up
            case 1:  x_--; y_++; break; // Up-Right
            case 2:           y_++; break; // Right
            case 3:  x_++; y_++; break; // Down-Right
            case 4:  x_++;      break; // Down
            case 5:  x_++; y_--; break; // Down-Left
            case 6:           y_--; break; // Left
            case 7:  x_--; y_--; break; // Up-Left
            default: break;
        }
    }

private:
    std::size_t x_, y_;
    int direction_;
};
