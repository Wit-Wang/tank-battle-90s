#pragma once

#include "ecs/Component.h"
#include "core/Types.h"
#include "raylib.h"
#include <functional>

/// 碰撞层：用于过滤不需要检测的碰撞对
enum class CollisionLayer : uint8_t {
    None     = 0,
    Player   = 1,
    Enemy    = 2,
    Bullet   = 4,
    Wall     = 8,
    Base     = 16,
    PowerUp  = 32,
};

inline CollisionLayer operator|(CollisionLayer a, CollisionLayer b) {
    return static_cast<CollisionLayer>(
        static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline bool HasLayer(CollisionLayer mask, CollisionLayer layer) {
    return (static_cast<uint8_t>(mask) & static_cast<uint8_t>(layer)) != 0;
}

/// 碰撞体组件：AABB 碰撞检测
class ColliderComponent : public Component {
public:
    ColliderComponent() = default;
    ColliderComponent(CollisionLayer layer, Vector2 size)
        : layer(layer), size(size) {}

    void Update(float dt) override;

    /// 获取世界坐标下的 AABB
    Rectangle GetAABB() const;

    /// 碰撞回调
    std::function<void(Entity* other)> onCollision;

    CollisionLayer layer       = CollisionLayer::None;
    CollisionLayer collidesWith = CollisionLayer::None;
    Vector2        size        { 0.f, 0.f };
    Vector2        offset      { 0.f, 0.f };
    int            team        = -1;  // -1 = 无队伍 (不参与队伍过滤)
};
