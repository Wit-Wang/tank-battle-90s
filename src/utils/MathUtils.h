#pragma once

#include "core/Types.h"
#include <cmath>

/// 方向转单位向量
inline Vector2 DirectionToVector(Direction dir) {
    switch (dir) {
        case Direction::UP:    return { 0.f, -1.f };
        case Direction::DOWN:  return { 0.f,  1.f };
        case Direction::LEFT:  return {-1.f,  0.f };
        case Direction::RIGHT: return { 1.f,  0.f };
    }
    return { 0.f, 0.f };
}

/// 方向转旋转角度 (度)
inline float DirectionToAngle(Direction dir) {
    switch (dir) {
        case Direction::UP:    return 0.f;
        case Direction::RIGHT: return 90.f;
        case Direction::DOWN:  return 180.f;
        case Direction::LEFT:  return 270.f;
    }
    return 0.f;
}
