#pragma once

#include "ecs/EntityManager.h"

class GameMap;

/// 碰撞检测系统：统一处理所有实体间的碰撞
class CollisionSystem {
public:
    /// 检测并处理所有碰撞
    static void Update(EntityManager& em, GameMap& map, float dt);

private:
    /// 检测两个实体是否碰撞
    static bool CheckCollision(Entity* a, Entity* b);

    /// 处理子弹与地图瓦片的碰撞
    static void BulletMapCollision(EntityManager& em, GameMap& map);

    /// 处理坦克与地图的碰撞
    static void TankMapCollision(EntityManager& em, GameMap& map);

    /// 处理实体间碰撞
    static void EntityEntityCollision(EntityManager& em);
};
