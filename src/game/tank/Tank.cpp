#include "Tank.h"
#include "ecs/EntityManager.h"
#include "ecs/components/TransformComponent.h"
#include "ecs/components/SpriteComponent.h"
#include "ecs/components/ColliderComponent.h"
#include "ecs/components/HealthComponent.h"
#include "ecs/components/MovementComponent.h"
#include "game/entities/Bullet.h"
#include "core/EventSystem.h"
#include "core/ResourceManager.h"
#include "core/AudioManager.h"
#include "utils/MathUtils.h"

Tank::Tank(TankType type, std::unique_ptr<IController> controller, int team)
    : type_(type), team_(team), controller_(std::move(controller)) {
    const auto& stats = GetTankStats(type);
    fireCooldown_.SetDuration(stats.fireCooldown);
}

void Tank::Spawn(EntityManager& em, Vector2 position) {
    const auto& stats = GetTankStats(type_);
    em_ = &em;

    entity_ = em.CreateEntity<Entity>();
    entity_->AddComponent<TransformComponent>(position);
    entity_->AddComponent<MovementComponent>(stats.moveSpeed);
    entity_->AddComponent<HealthComponent>(stats.maxHealth);
    entity_->AddComponent<ColliderComponent>(
        CollisionLayer::Player, Vector2{ TILE_SIZE * 0.8f, TILE_SIZE * 0.8f });

    // 精灵图 (程序化生成的纹理)
    static const char* typeNames[] = { "light", "medium", "heavy", "speed" };
    std::string texName = std::string("tank_") + typeNames[static_cast<int>(type_)] + "_team" + std::to_string(team_);
    auto& rm = ResourceManager::Instance();
    if (rm.HasTexture(texName)) {
        entity_->AddComponent<SpriteComponent>(&rm.GetTexture(texName));
    }

    // 设置碰撞队伍
    auto* col = entity_->GetComponent<ColliderComponent>();
    col->team = team_;
    col->collidesWith = CollisionLayer::Bullet | CollisionLayer::Wall | CollisionLayer::Base;

    // 碰撞回调: 被子弹击中时受伤
    Tank* self = this;
    col->onCollision = [self](Entity* other) {
        auto* bullet = dynamic_cast<Bullet*>(other);
        if (bullet && bullet->GetOwnerTeam() != self->team_) {
            self->TakeDamage(bullet->GetDamage());
            bullet->SetActive(false);
        }
    };

    // 出生无敌 2 秒
    SetInvincible(2.f);

    // 死亡回调
    auto* health = entity_->GetComponent<HealthComponent>();
    health->onDeath = [this]() {
        EventSystem::Instance().Dispatch({
            EventType::TankDestroyed, entity_->GetId(), team_
        });
        AudioManager::Instance().PlaySound("explosion");
        // 停用实体，使其不再参与碰撞和更新
        entity_->SetActive(false);
    };
}

void Tank::Update(float dt) {
    if (!entity_ || !entity_->IsActive()) return;

    controller_->Update(*this, dt);
    fireCooldown_.Update(dt);

    // 根据移动方向旋转坦克
    auto* movement = entity_->GetComponent<MovementComponent>();
    auto* transform = entity_->GetComponent<TransformComponent>();
    if (movement && transform) {
        transform->SetRotation(DirectionToAngle(movement->GetDirection()));

        // 地图边界钳制 (坦克不能走出地图)
        float halfSize = TILE_SIZE * 0.4f;
        Vector2 pos = transform->GetPosition();
        if (pos.x < halfSize) pos.x = halfSize;
        if (pos.x > MAP_COLS * TILE_SIZE - halfSize) pos.x = MAP_COLS * TILE_SIZE - halfSize;
        if (pos.y < halfSize) pos.y = halfSize;
        if (pos.y > MAP_ROWS * TILE_SIZE - halfSize) pos.y = MAP_ROWS * TILE_SIZE - halfSize;
        transform->SetPosition(pos);
    }

    if (invincible_) {
        invincibleTimer_.Update(dt);
        if (invincibleTimer_.IsFinished()) {
            invincible_ = false;
            // 重置 tint，防止残留半透明
            auto* sprite = entity_->GetComponent<SpriteComponent>();
            if (sprite) sprite->tint = WHITE;
        }
    }

    // 保存移动前位置，供碰撞系统判断移动轴
    if (transform) transform->SavePosition();

    entity_->Update(dt);
}

void Tank::Render() {
    if (!entity_ || !entity_->IsActive()) return;

    // 无敌闪烁效果
    if (invincible_) {
        auto* sprite = entity_->GetComponent<SpriteComponent>();
        if (sprite) {
            float alpha = std::abs(std::sin(GetTime() * 10.f));
            sprite->tint = ColorAlpha(WHITE, alpha);
        }
    }

    entity_->Render();
}

void Tank::Fire() {
    if (!fireCooldown_.IsFinished()) return;
    if (!entity_ || !em_) return;

    const auto& stats = GetTankStats(type_);

    // 获取坦克朝向和位置
    auto* movement = entity_->GetComponent<MovementComponent>();
    auto* transform = entity_->GetComponent<TransformComponent>();
    if (!movement || !transform) return;

    Direction dir = movement->GetDirection();
    Vector2 dirVec = DirectionToVector(dir);
    // 子弹从坦克前方发射
    Vector2 spawnPos = {
        transform->GetPosition().x + dirVec.x * (TILE_SIZE * 0.5f),
        transform->GetPosition().y + dirVec.y * (TILE_SIZE * 0.5f)
    };

    // 创建子弹实体
    auto* bullet = em_->CreateEntity<Bullet>(team_, stats.bulletDamage, stats.bulletSpeed, dir);
    bullet->AddComponent<TransformComponent>(spawnPos);
    bullet->AddComponent<ColliderComponent>(
        CollisionLayer::Bullet, Vector2{ 8.f, 8.f });

    // 子弹纹理
    std::string bulletTex = std::string("bullet_team") + std::to_string(team_);
    if (ResourceManager::Instance().HasTexture(bulletTex)) {
        bullet->AddComponent<SpriteComponent>(&ResourceManager::Instance().GetTexture(bulletTex));
    }

    // 设置子弹碰撞掩码: 与 Wall、Base、Player/Enemy 碰撞
    auto* col = bullet->GetComponent<ColliderComponent>();
    col->collidesWith = CollisionLayer::Wall | CollisionLayer::Base |
                        CollisionLayer::Player | CollisionLayer::Enemy;
    col->team = team_;  // 用于同队子弹过滤

    fireCooldown_.Reset();

    // 播放射击音效
    AudioManager::Instance().PlaySound("shoot");
}

void Tank::TakeDamage(int amount) {
    if (invincible_) return;

    const auto& stats = GetTankStats(type_);
    int effective = std::max(1, amount - stats.armor);
    auto* health = entity_->GetComponent<HealthComponent>();
    if (health) {
        health->TakeDamage(effective);
    }
}

bool Tank::IsDead() const {
    auto* health = entity_ ? entity_->GetComponent<HealthComponent>() : nullptr;
    return !health || health->IsDead();
}

const TankStats& Tank::GetStats() const {
    return TANK_STATS.at(type_);
}

void Tank::SetInvincible(float duration) {
    invincible_ = true;
    invincibleTimer_.SetDuration(duration);
    invincibleTimer_.Reset();
}
