#pragma once

#include "ecs/Entity.h"
#include "core/Types.h"

/// 道具类型
enum class PowerUpType : uint8_t {
    STAR,      // 射速提升
    BOMB,      // 全屏清敌
    SHOVEL,    // 基地加固
    TANK,      // 加命
    HELMET,    // 无敌
};

/// 道具实体
class PowerUp : public Entity {
public:
    PowerUp(int id, PowerUpType type, Vector2 position);

    void Update(float dt) override;
    void Render() override;

    PowerUpType GetType() const { return type_; }

private:
    PowerUpType type_;
    float lifetime_ = 15.f;  // 15 秒后消失
    float blinkTimer_ = 0.f;
};
