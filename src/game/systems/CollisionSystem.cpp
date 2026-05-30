#include "CollisionSystem.h"
#include "ecs/components/ColliderComponent.h"
#include "ecs/components/TransformComponent.h"
#include "game/entities/Bullet.h"
#include "game/entities/Wall.h"
#include "game/entities/Base.h"
#include "game/entities/PowerUp.h"
#include "game/map/GameMap.h"
#include "core/EventSystem.h"
#include "utils/MathUtils.h"
#include <cmath>

void CollisionSystem::Update(EntityManager& em, GameMap& map, float dt) {
    BulletMapCollision(em, map);
    TankMapCollision(em, map);
    EntityEntityCollision(em);
}

bool CollisionSystem::CheckCollision(Entity* a, Entity* b) {
    auto* colA = a->GetComponent<ColliderComponent>();
    auto* colB = b->GetComponent<ColliderComponent>();
    if (!colA || !colB) return false;

    // 层过滤
    if (!HasLayer(colA->collidesWith, colB->layer) &&
        !HasLayer(colB->collidesWith, colA->layer))
        return false;

    Rectangle rectA = colA->GetAABB();
    Rectangle rectB = colB->GetAABB();
    return CheckCollisionRecs(rectA, rectB);
}

void CollisionSystem::BulletMapCollision(EntityManager& em, GameMap& map) {
    em.ForEach([&](Entity* e) {
        auto* bullet = dynamic_cast<Bullet*>(e);
        if (!bullet || !bullet->IsActive()) return;

        auto* transform = bullet->GetComponent<TransformComponent>();
        if (!transform) return;

        // 出界检查
        float px = transform->GetPosition().x;
        float py = transform->GetPosition().y;
        if (px < 0 || px >= MAP_COLS * TILE_SIZE || py < 0 || py >= MAP_ROWS * TILE_SIZE) {
            bullet->SetActive(false);
            return;
        }

        // 子弹所在瓦片
        int tc = static_cast<int>(px) / TILE_SIZE;
        int tr = static_cast<int>(py) / TILE_SIZE;

        TileType tile = map.GetTile(tc, tr);
        if (tile == TileType::BRICK) {
            map.SetTile(tc, tr, TileType::EMPTY);
            bullet->SetActive(false);
            Event e{ EventType::WallDestroyed, bullet->GetId(), 0 };
            e.x = tc * TILE_SIZE + TILE_SIZE / 2.f;
            e.y = tr * TILE_SIZE + TILE_SIZE / 2.f;
            EventSystem::Instance().Dispatch(e);
        } else if (tile == TileType::STEEL) {
            bullet->SetActive(false);
        } else if (tile == TileType::WATER) {
            // 子弹可以飞过水面
        }
    });
}

void CollisionSystem::TankMapCollision(EntityManager& em, GameMap& map) {
    em.ForEach([&](Entity* e) {
        auto* transform = e->GetComponent<TransformComponent>();
        auto* col = e->GetComponent<ColliderComponent>();
        if (!transform || !col) return;

        // 独立解析 X 和 Y 轴，防止对角穿墙
        for (int axis = 0; axis < 2; axis++) {
            Rectangle aabb = col->GetAABB();
            int minC = static_cast<int>(aabb.x + 0.001f) / TILE_SIZE;
            int maxC = static_cast<int>(aabb.x + aabb.width - 0.001f) / TILE_SIZE;
            int minR = static_cast<int>(aabb.y + 0.001f) / TILE_SIZE;
            int maxR = static_cast<int>(aabb.y + aabb.height - 0.001f) / TILE_SIZE;

            // 分组收集所有墙壁的推离量，防止推入相邻墙壁
            float pushNeg = 0.f; // 向负方向推 (左/上) 的最大量
            float pushPos = 0.f; // 向正方向推 (右/下) 的最大量

            for (int r = minR; r <= maxR; r++) {
                for (int c = minC; c <= maxC; c++) {
                    if (!map.IsWalkable(c, r)) {
                        Rectangle wallRect = {
                            static_cast<float>(c * TILE_SIZE),
                            static_cast<float>(r * TILE_SIZE),
                            static_cast<float>(TILE_SIZE),
                            static_cast<float>(TILE_SIZE)
                        };
                        Rectangle overlap = GetCollisionRec(aabb, wallRect);

                        Vector2 pos = transform->GetPosition();
                        if (axis == 0 && overlap.width > 0) {
                            float wallCenter = c * TILE_SIZE + TILE_SIZE / 2.f;
                            if (pos.x < wallCenter)
                                pushNeg = std::max(pushNeg, overlap.width);
                            else
                                pushPos = std::max(pushPos, overlap.width);
                        } else if (axis == 1 && overlap.height > 0) {
                            float wallCenter = r * TILE_SIZE + TILE_SIZE / 2.f;
                            if (pos.y < wallCenter)
                                pushNeg = std::max(pushNeg, overlap.height);
                            else
                                pushPos = std::max(pushPos, overlap.height);
                        }
                    }
                }
            }

            // 应用净推离量
            if (pushNeg > 0 || pushPos > 0) {
                Vector2 pos = transform->GetPosition();
                if (axis == 0)
                    pos.x += pushPos - pushNeg;
                else
                    pos.y += pushPos - pushNeg;
                transform->SetPosition(pos);
            }
        }
    });
}

void CollisionSystem::EntityEntityCollision(EntityManager& em) {
    // Snapshot entity count — safe even if callbacks add new entities
    const auto& entities = em.GetEntities();
    size_t count = entities.size();

    for (size_t i = 0; i < count; i++) {
        if (!entities[i] || !entities[i]->IsActive()) continue;
        for (size_t j = i + 1; j < count; j++) {
            if (!entities[j] || !entities[j]->IsActive()) continue;

            auto* colA = entities[i]->GetComponent<ColliderComponent>();
            auto* colB = entities[j]->GetComponent<ColliderComponent>();
            if (!colA || !colB) continue;

            // Skip same-team bullet-tank collisions
            if (colA->team >= 0 && colB->team >= 0 && colA->team == colB->team) {
                bool aIsBullet = dynamic_cast<Bullet*>(entities[i].get()) != nullptr;
                bool bIsBullet = dynamic_cast<Bullet*>(entities[j].get()) != nullptr;
                if (aIsBullet || bIsBullet) continue;
            }

            if (CheckCollision(entities[i].get(), entities[j].get())) {
                // Tank-tank: push apart, no onCollision
                bool aIsPlayer = colA->layer == CollisionLayer::Player;
                bool bIsPlayer = colB->layer == CollisionLayer::Player;
                if (aIsPlayer && bIsPlayer) {
                    auto* tA = entities[i]->GetComponent<TransformComponent>();
                    auto* tB = entities[j]->GetComponent<TransformComponent>();
                    if (tA && tB) {
                        Vector2 posA = tA->GetPosition();
                        Vector2 posB = tB->GetPosition();
                        float dx = posB.x - posA.x;
                        float dy = posB.y - posA.y;
                        float dist = std::sqrt(dx * dx + dy * dy);
                        if (dist < 1.f) dist = 1.f;
                        float overlap = (TILE_SIZE * 0.8f) - dist;
                        if (overlap > 0) {
                            float pushX = (dx / dist) * overlap * 0.5f;
                            float pushY = (dy / dist) * overlap * 0.5f;
                            tA->SetPosition({ posA.x - pushX, posA.y - pushY });
                            tB->SetPosition({ posB.x + pushX, posB.y + pushY });
                        }
                    }
                    continue;
                }

                if (colA->onCollision) colA->onCollision(entities[j].get());
                if (colB->onCollision) colB->onCollision(entities[i].get());
            }
        }
    }
}
