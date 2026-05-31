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

        // 出界检查 (使用整数安全比较，避免 float→int 溢出)
        float px = transform->GetPosition().x;
        float py = transform->GetPosition().y;
        float maxX = static_cast<float>(MAP_COLS * TILE_SIZE);
        float maxY = static_cast<float>(MAP_ROWS * TILE_SIZE);
        if (px < 0.f || px >= maxX || py < 0.f || py >= maxY) {
            bullet->SetActive(false);
            return;
        }

        // 安全计算瓦片坐标 (先除后转 int，避免大值溢出)
        int tc = static_cast<int>(px / TILE_SIZE);
        int tr = static_cast<int>(py / TILE_SIZE);

        TileType tile = map.GetTile(tc, tr);
        if (tile == TileType::BRICK) {
            map.SetTile(tc, tr, TileType::EMPTY);
            bullet->SetActive(false);
            Event evt{ EventType::WallDestroyed, bullet->GetId(), 0 };
            evt.x = tc * TILE_SIZE + TILE_SIZE / 2.f;
            evt.y = tr * TILE_SIZE + TILE_SIZE / 2.f;
            EventSystem::Instance().Dispatch(evt);
        } else if (tile == TileType::STEEL) {
            bullet->SetActive(false);
        } else if (tile == TileType::WATER) {
            // 子弹可以飞过水面
        }
    });
}

void CollisionSystem::TankMapCollision(EntityManager& em, GameMap& map) {
    em.ForEach([&](Entity* e) {
        if (!e->IsActive()) return;
        auto* transform = e->GetComponent<TransformComponent>();
        auto* col = e->GetComponent<ColliderComponent>();
        if (!transform || !col) return;

        // 只处理坦克 (Player 层)，子弹/道具等不参与墙壁推离
        if (col->layer != CollisionLayer::Player) return;

        // 根据本帧实际移动方向决定解析哪些轴
        Vector2 curPos = transform->GetPosition();
        Vector2 prevPos = transform->GetPrevPosition();
        float dx = curPos.x - prevPos.x;
        float dy = curPos.y - prevPos.y;
        bool movedX = std::abs(dx) > 0.001f;
        bool movedY = std::abs(dy) > 0.001f;

        // 未移动则跳过
        if (!movedX && !movedY) return;

        // 安全获取瓦片范围 (clamp 到地图边界，避免越界)
        auto GetTileRange = [](float lo, float hi, int maxTile) {
            int minT = std::max(0, static_cast<int>(lo) / TILE_SIZE);
            int maxT = std::min(maxTile - 1, static_cast<int>(hi) / TILE_SIZE);
            return std::pair{ minT, maxT };
        };

        // 分轴独立解析：先修正 X，再修正 Y（使用修正后的位置重新计算 AABB）
        // Pass 1: X 轴（仅当本帧有水平移动时解析）
        if (movedX) {
            Rectangle aabb = col->GetAABB();
            auto [minC, maxC] = GetTileRange(aabb.x + 0.001f, aabb.x + aabb.width - 0.001f, MAP_COLS);
            auto [minR, maxR] = GetTileRange(aabb.y + 0.001f, aabb.y + aabb.height - 0.001f, MAP_ROWS);

            float pushNeg = 0.f, pushPos = 0.f;
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
                        if (overlap.width > 0) {
                            Vector2 pos = transform->GetPosition();
                            float wallCenter = c * TILE_SIZE + TILE_SIZE / 2.f;
                            if (pos.x < wallCenter)
                                pushNeg = std::max(pushNeg, overlap.width);
                            else
                                pushPos = std::max(pushPos, overlap.width);
                        }
                    }
                }
            }
            if (pushNeg > 0 || pushPos > 0) {
                Vector2 pos = transform->GetPosition();
                pos.x += pushPos - pushNeg;
                transform->SetPosition(pos);
            }
        }

        // Pass 2: Y 轴（仅当本帧有垂直移动时解析，使用 X 修正后的位置）
        if (movedY) {
            Rectangle aabb = col->GetAABB();
            auto [minC, maxC] = GetTileRange(aabb.x + 0.001f, aabb.x + aabb.width - 0.001f, MAP_COLS);
            auto [minR, maxR] = GetTileRange(aabb.y + 0.001f, aabb.y + aabb.height - 0.001f, MAP_ROWS);

            float pushNeg = 0.f, pushPos = 0.f;
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
                        if (overlap.height > 0) {
                            Vector2 pos = transform->GetPosition();
                            float wallCenter = r * TILE_SIZE + TILE_SIZE / 2.f;
                            if (pos.y < wallCenter)
                                pushNeg = std::max(pushNeg, overlap.height);
                            else
                                pushPos = std::max(pushPos, overlap.height);
                        }
                    }
                }
            }
            if (pushNeg > 0 || pushPos > 0) {
                Vector2 pos = transform->GetPosition();
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
                // 回调可能停用了 entities[i] (如 BOMB 道具连环击杀)，需重新检查
                if (!entities[i]->IsActive()) break;
                if (colB->onCollision) colB->onCollision(entities[i].get());
            }
        }
    }
}
