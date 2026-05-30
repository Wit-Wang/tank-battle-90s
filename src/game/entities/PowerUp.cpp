#include "PowerUp.h"
#include "ecs/components/TransformComponent.h"
#include "ecs/components/ColliderComponent.h"

PowerUp::PowerUp(int id, PowerUpType type, Vector2 position)
    : Entity(id), type_(type) {
    AddComponent<TransformComponent>(position);
    AddComponent<ColliderComponent>(CollisionLayer::PowerUp, Vector2{ TILE_SIZE * 0.8f, TILE_SIZE * 0.8f });
}

void PowerUp::Update(float dt) {
    lifetime_ -= dt;
    if (lifetime_ <= 0.f) {
        SetActive(false);
        return;
    }
    blinkTimer_ += dt;
    Entity::Update(dt);
}

void PowerUp::Render() {
    auto* transform = GetComponent<TransformComponent>();
    if (!transform) return;

    // 快消失时闪烁
    if (lifetime_ < 5.f && std::sin(blinkTimer_ * 8.f) < 0.f) return;

    float x = transform->GetPosition().x - TILE_SIZE / 2;
    float y = transform->GetPosition().y - TILE_SIZE / 2;

    Color color;
    const char* icon;
    switch (type_) {
        case PowerUpType::STAR:   color = YELLOW;   icon = "S"; break;
        case PowerUpType::BOMB:   color = RED;      icon = "B"; break;
        case PowerUpType::SHOVEL: color = BROWN;    icon = "H"; break;
        case PowerUpType::TANK:   color = GREEN;    icon = "T"; break;
        case PowerUpType::HELMET: color = SKYBLUE;  icon = "X"; break;
    }

    DrawRectangle(static_cast<int>(x), static_cast<int>(y), TILE_SIZE, TILE_SIZE, color);
    int tw = MeasureText(icon, 24);
    DrawText(icon,
        static_cast<int>(transform->GetPosition().x - tw / 2),
        static_cast<int>(transform->GetPosition().y - 12),
        24, WHITE);

    Entity::Render();
}
