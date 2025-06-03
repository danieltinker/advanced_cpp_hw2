#include "Tank.h"
#include <algorithm> // for std::clamp, if needed

using namespace arena;
using namespace common;

Tank::Tank(int playerIndex, size_t x, size_t y, size_t maxSteps, size_t numShells)
    : playerIndex_(playerIndex),
      x_(x),
      y_(y),
      maxSteps_(maxSteps),
      shellsRemaining_(numShells),
      cooldown_(0),
      alive_(true),
      directionIndex_( (playerIndex == 1) ? 6 : 2 )  // 6 = left for P1, 2 = right for P2
{
}

Tank::~Tank() = default;

int Tank::getPlayerIndex() const {
    return playerIndex_;
}

size_t Tank::getX() const {
    return x_;
}

size_t Tank::getY() const {
    return y_;
}

bool Tank::isAlive() const {
    return alive_;
}

size_t Tank::getShellsRemaining() const {
    return shellsRemaining_;
}

int Tank::getCooldown() const {
    return cooldown_;
}

int Tank::getDirectionIndex() const {
    return directionIndex_;
}

void Tank::applyAction(ActionRequest action) {
    if (!alive_) return;

    switch (action) {
        case ActionRequest::MoveForward: {
            // Step one cell in the current direction
            int newX = static_cast<int>(x_) + DX[directionIndex_];
            int newY = static_cast<int>(y_) + DY[directionIndex_];
            if (newX >= 0 && newY >= 0) {
                x_ = static_cast<size_t>(newX);
                y_ = static_cast<size_t>(newY);
            }
            break;
        }
        case ActionRequest::MoveBackward: {
            // Step one cell opposite direction: add 4 mod 8 to get opposite
            int backDir = (directionIndex_ + 4) & 7;
            int newX = static_cast<int>(x_) + DX[backDir];
            int newY = static_cast<int>(y_) + DY[backDir];
            if (newX >= 0 && newY >= 0) {
                x_ = static_cast<size_t>(newX);
                y_ = static_cast<size_t>(newY);
            }
            break;
        }
        case ActionRequest::RotateLeft90:
            // Turn left by 90° ⇒ subtract 2 (mod 8)
            directionIndex_ = (directionIndex_ + 8 - 2) & 7;
            break;

        case ActionRequest::RotateRight90:
            // Turn right by 90° ⇒ add 2 (mod 8)
            directionIndex_ = (directionIndex_ + 2) & 7;
            break;

        case ActionRequest::RotateLeft45:
            // Turn left by 45° ⇒ subtract 1 (mod 8)
            directionIndex_ = (directionIndex_ + 8 - 1) & 7;
            break;

        case ActionRequest::RotateRight45:
            // Turn right by 45° ⇒ add 1 (mod 8)
            directionIndex_ = (directionIndex_ + 1) & 7;
            break;

        case ActionRequest::Shoot:
            if (cooldown_ == 0 && shellsRemaining_ > 0) {
                shellsRemaining_--;
                cooldown_ = 3; // e.g. a fixed 3‐tick cooldown (adjust as desired)
            }
            break;

        case ActionRequest::GetBattleInfo:
        case ActionRequest::DoNothing:
            // no position/orientation change here
            break;
    }
}

void Tank::tickCooldown() {
    if (cooldown_ > 0) {
        cooldown_--;
    }
}

void Tank::destroy() {
    alive_ = false;
}
