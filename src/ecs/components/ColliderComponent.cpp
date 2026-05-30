#include "ColliderComponent.h"
#include "ecs/Entity.h"
#include "TransformComponent.h"

void ColliderComponent::Update(float dt) {
    // 碰撞检测由 CollisionSystem 统一处理
}

Rectangle ColliderComponent::GetAABB() const {
    auto* transform = GetOwner()->GetComponent<TransformComponent>();
    if (!transform) return { 0, 0, size.x, size.y };

    return {
        transform->GetPosition().x + offset.x - size.x / 2.f,
        transform->GetPosition().y + offset.y - size.y / 2.f,
        size.x,
        size.y
    };
}
