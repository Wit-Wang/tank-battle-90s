#include "Base.h"
#include "ecs/components/TransformComponent.h"
#include "ecs/components/ColliderComponent.h"
#include "ecs/components/HealthComponent.h"
#include "ecs/components/SpriteComponent.h"
#include "core/EventSystem.h"
#include "core/ResourceManager.h"

Base::Base(int id, Vector2 position, int team) : Entity(id), team_(team) {
    AddComponent<TransformComponent>(position);
    AddComponent<ColliderComponent>(CollisionLayer::Base, Vector2{ TILE_SIZE, TILE_SIZE });
    AddComponent<HealthComponent>(1);

    // 加载纹理
    std::string texName = std::string("base_team") + std::to_string(team_);
    auto& rm = ResourceManager::Instance();
    if (rm.HasTexture(texName)) {
        AddComponent<SpriteComponent>(&rm.GetTexture(texName));
    }
}

void Base::Render() {
    // 更新纹理 (被摧毁后切换到废墟纹理)
    auto* sprite = GetComponent<SpriteComponent>();
    if (sprite) {
        std::string texName = std::string("base_team") + std::to_string(team_)
                            + (destroyed_ ? "_destroyed" : "");
        auto& rm = ResourceManager::Instance();
        if (rm.HasTexture(texName)) {
            sprite->texture = &rm.GetTexture(texName);
        }
    }

    Entity::Render();

    // 无纹理回退
    if (!sprite || !sprite->texture) {
        auto* transform = GetComponent<TransformComponent>();
        if (!transform) return;

        Color color = destroyed_ ? DARKGRAY : GOLD;
        float x = transform->GetPosition().x - TILE_SIZE / 2;
        float y = transform->GetPosition().y - TILE_SIZE / 2;
        DrawRectangle(static_cast<int>(x), static_cast<int>(y), TILE_SIZE, TILE_SIZE, color);
        DrawRectangleLines(static_cast<int>(x), static_cast<int>(y), TILE_SIZE, TILE_SIZE, YELLOW);
    }
}

void Base::Destroy() {
    if (destroyed_) return;
    destroyed_ = true;
    EventSystem::Instance().Dispatch({ EventType::BaseDestroyed, GetId(), 0 });
}
