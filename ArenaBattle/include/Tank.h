#pragma once

#include "common/ActionRequest.h"

/*
 A Tank wraps a TankAlgorithm (AI) and has position, direction, shells, etc.
 applyAction(...) will take an ActionRequest and update the tank's own state
 (position/direction) if legal, or note that it was “ignored” if illegal.
*/
class Tank {
public:
    Tank(int playerIndex, int tankIndex)
        : playerIndex_(playerIndex),
          tankIndex_(tankIndex),
          x_(0), y_(0),
          direction_((playerIndex == 1) ? 6 : 2), // Player1 faces Left(6), Player2 faces Right(2)
          alive_(false),
          shellsLeft_(0)
    {}

    void setPosition(std::size_t r, std::size_t c) {
        x_ = r; y_ = c; alive_ = true;
    }
    void setShells(std::size_t s) { shellsLeft_ = s; }

    std::size_t getX() const { return x_; }
    std::size_t getY() const { return y_; }
    int getDirection() const { return direction_; }
    bool isAlive() const { return alive_; }
    std::size_t getShellsLeft() const { return shellsLeft_; }
    int getPlayerIndex() const { return playerIndex_; }
    int getTankIndex() const { return tankIndex_; }

    // Called by GameManager to apply a single ActionRequest to this tank.
    void applyAction(common::ActionRequest action) {
        // 1) If action == RotateLeft90, direction_ = (direction_ + 6) % 8, etc.
        // 2) If MoveForward: compute (newR,newC) based on direction_, etc.
        // 3) If Shoot: decrement shellsLeft_ if >0; GameManager handles actual shell spawn.
        // 4) If GetBattleInfo or DoNothing: do not change position/direction here.
        //    (GetBattleInfo is handled entirely by GM/Player → not by this method.)
        //
        // The actual movement/ignorance logic is handled in GameManager when it resolves moves:
        // this function may simply record the intended “direction_” change or note “wantsToShoot.”
        //
        // For brevity, implementation details are in GameManager; this stub exists so
        // that compile passes.
    }

    void kill() { alive_ = false; }

private:
    int playerIndex_;
    int tankIndex_;
    std::size_t x_, y_;
    int direction_;        // 0..7
    bool alive_;
    std::size_t shellsLeft_;
};
