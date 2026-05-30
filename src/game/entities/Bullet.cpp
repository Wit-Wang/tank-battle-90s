#include "Bullet.h"
#include "ecs/components/TransformComponent.h"
#include "ecs/components/ColliderComponent.h"
#include "ecs/components/SpriteComponent.h"
#include "core/ResourceManager.h"
#include "utils/MathUtils.h"

Bullet::Bullet(int id, int ownerTeam, int damage, float speed, Direction dir)
    : Entity(id), ownerTeam_(ownerTeam), damage_(damage), speed_(speed), direction_(dir) {}

void Bullet::Update(float dt) {
    lifetime_ += dt;
    if (lifetime_ >= maxLifetime_) {
        SetActive(false);
        return;
    }

    auto* transform = GetComponent<TransformComponent>();
    if (!transform) return;

    Vector2 dirVec = DirectionToVector(direction_);
    Vector2 pos = transform->GetPosition();
    pos.x += dirVec.x * speed_ * dt;
    pos.y += dirVec.y * speed_ * dt;
    transform->SetPosition(pos);
    transform->SetRotation(DirectionToAngle(direction_));

    Entity::Update(dt);
}

void Bullet::Render() {
    auto* sprite = GetComponent<SpriteComponent>();
    if (sprite && sprite->texture) {
        Entity::Render();  // SpriteComponent 处理渲染
        return;
    }

    // 无纹理时回退到颜色方块
    auto* transform = GetComponent<TransformComponent>();
    if (!transform) return;

    float size = 8.f;
    DrawRectangle(
        static_cast<int>(transform->GetPosition().x - size / 2),
        static_cast<int>(transform->GetPosition().y - size / 2),
        static_cast<int>(size), static_cast<int>(size),
        YELLOW);

    Entity::Render();
}
