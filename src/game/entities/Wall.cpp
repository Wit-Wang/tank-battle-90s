#include "Wall.h"
#include "game/map/GameMap.h"
#include "ecs/components/TransformComponent.h"
#include "ecs/components/ColliderComponent.h"

Wall::Wall(int id, TileType type, int col, int row)
    : Entity(id), tileType_(type), gridCol_(col), gridRow_(row) {
    float x = col * TILE_SIZE + TILE_SIZE / 2.f;
    float y = row * TILE_SIZE + TILE_SIZE / 2.f;
    AddComponent<TransformComponent>(Vector2{ x, y });
    AddComponent<ColliderComponent>(CollisionLayer::Wall, Vector2{ TILE_SIZE, TILE_SIZE });

    hp_ = (type == TileType::BRICK) ? 1 : -1;  // -1 = 不可破坏
}

void Wall::Render() {
    auto* transform = GetComponent<TransformComponent>();
    if (!transform) return;

    Color color = GameMap::GetTileColor(tileType_);
    DrawRectangle(
        static_cast<int>(transform->GetPosition().x - TILE_SIZE / 2),
        static_cast<int>(transform->GetPosition().y - TILE_SIZE / 2),
        TILE_SIZE, TILE_SIZE, color);

    if (tileType_ == TileType::BRICK) {
        DrawRectangleLines(
            static_cast<int>(transform->GetPosition().x - TILE_SIZE / 2),
            static_cast<int>(transform->GetPosition().y - TILE_SIZE / 2),
            TILE_SIZE, TILE_SIZE, DARKBROWN);
    }

    Entity::Render();
}

bool Wall::OnHit() {
    if (hp_ < 0) return false;  // 不可破坏
    hp_--;
    if (hp_ <= 0) {
        SetActive(false);
        return true;  // 已被摧毁
    }
    return false;
}
