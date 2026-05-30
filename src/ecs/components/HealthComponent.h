#pragma once

#include "ecs/Component.h"
#include <functional>

/// 生命值组件：管理 HP、受伤、死亡
class HealthComponent : public Component {
public:
    HealthComponent() = default;
    explicit HealthComponent(int maxHp) : maxHp_(maxHp), hp_(maxHp) {}

    void TakeDamage(int amount);
    void Heal(int amount);
    bool IsDead() const { return hp_ <= 0; }

    int GetHp()    const { return hp_; }
    int GetMaxHp() const { return maxHp_; }

    /// 死亡回调
    std::function<void()> onDeath;

private:
    int maxHp_ = 1;
    int hp_    = 1;
};
