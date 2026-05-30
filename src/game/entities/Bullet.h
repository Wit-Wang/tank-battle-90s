#pragma once

#include "ecs/Entity.h"
#include "core/Types.h"

/// 子弹实体
class Bullet : public Entity {
public:
    Bullet(int id, int ownerTeam, int damage, float speed, Direction dir);

    void Update(float dt) override;
    void Render() override;

    int       GetOwnerTeam() const { return ownerTeam_; }
    int       GetDamage()    const { return damage_; }
    Direction GetDirection()  const { return direction_; }

private:
    int       ownerTeam_;
    int       damage_;
    float     speed_;
    Direction direction_;
    float     lifetime_ = 0.f;
    float     maxLifetime_ = 5.f;
};
