#pragma once

#include "TankType.h"
#include "controller/IController.h"
#include "ecs/Entity.h"
#include "core/Timer.h"
#include "core/Types.h"
#include <memory>

class EntityManager;

/// 坦克基类：组合 Entity + Components + Controller
/// 所有坦克共用此类，通过 TankType 区分属性，通过 IController 区分控制方式
class Tank {
public:
    Tank(TankType type, std::unique_ptr<IController> controller, int team);
    ~Tank() = default;

    void Update(float dt);
    void Render();

    /// 开火
    void Fire();

    /// 受到伤害
    void TakeDamage(int amount);

    /// 死亡
    bool IsDead() const;

    /// 在 EntityManager 中创建并注册自身 Entity
    void Spawn(EntityManager& em, Vector2 position);

    // ---- Getters ----
    TankType   GetType()      const { return type_; }
    int        GetTeam()      const { return team_; }
    Entity*    GetEntity()    const { return entity_; }
    IController* GetController() const { return controller_.get(); }
    const TankStats& GetStats() const;
    bool IsInvincible() const { return invincible_; }

    void SetInvincible(float duration);

private:
    TankType type_;
    int team_;
    std::unique_ptr<IController> controller_;
    Entity* entity_ = nullptr;  // 由 EntityManager 拥有
    EntityManager* em_ = nullptr;  // 用于创建子弹

    Timer fireCooldown_{ 0.f };
    Timer invincibleTimer_{ 0.f };
    bool  invincible_ = false;
};
