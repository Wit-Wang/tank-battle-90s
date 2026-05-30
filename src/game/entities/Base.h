#pragma once

#include "ecs/Entity.h"
#include "core/Types.h"

/// 基地 (老鹰) 实体：被摧毁则游戏结束
class Base : public Entity {
public:
    Base(int id, Vector2 position, int team = 0);

    void Render() override;

    bool IsDestroyed() const { return destroyed_; }
    void Destroy();
    int GetTeam() const { return team_; }

private:
    int team_ = 0;
    bool destroyed_ = false;
};
