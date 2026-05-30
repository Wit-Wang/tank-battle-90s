#pragma once

#include "ecs/Component.h"
#include "core/Types.h"

/// 移动组件：控制实体的移动速度与方向
class MovementComponent : public Component {
public:
    MovementComponent() = default;
    explicit MovementComponent(float speed) : speed_(speed) {}

    void Update(float dt) override;

    void SetDirection(Direction dir) { direction_ = dir; }
    Direction GetDirection() const { return direction_; }

    void SetSpeed(float s) { speed_ = s; }
    float GetSpeed() const { return speed_; }

    void SetMoving(bool v) { moving_ = v; }
    bool IsMoving() const { return moving_; }

    /// 尝试移动，返回实际移动后的位置 (会考虑边界)
    Vector2 TryMove(Vector2 current, float dt) const;

private:
    float     speed_    = 100.f;
    Direction direction_ = Direction::UP;
    bool      moving_   = false;
};
