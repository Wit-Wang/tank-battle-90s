#include "AIController.h"
#include "game/tank/Tank.h"
#include "ecs/components/MovementComponent.h"
#include "ecs/components/TransformComponent.h"
#include "utils/Random.h"
#include <cmath>

const std::vector<Tank*>* AIController::allTanks_ = nullptr;
Entity* AIController::bases_[2] = { nullptr, nullptr };

AIController::AIController() {
    patrolDir_ = static_cast<Direction>(Random::Int(0, 3));
}

void AIController::Update(Tank& tank, float dt) {
    fireTimer_.Update(dt);

    switch (state_) {
        case AIState::PATROL:  UpdatePatrol(tank, dt); break;
        case AIState::CHASE:   UpdateChase(tank, dt);  break;
        case AIState::ATTACK:  UpdateAttack(tank, dt); break;
    }
}

void AIController::UpdatePatrol(Tank& tank, float dt) {
    auto* movement = tank.GetEntity()->GetComponent<MovementComponent>();
    if (!movement) return;

    dirChangeTimer_.Update(dt);
    if (dirChangeTimer_.IsFinished()) {
        patrolDir_ = static_cast<Direction>(Random::Int(0, 3));
        dirChangeTimer_.Reset();
    }

    movement->SetDirection(patrolDir_);
    movement->SetMoving(true);

    // 发现敌人 → 切换到追击
    if (FindNearestEnemy(tank)) {
        state_ = AIState::CHASE;
    }

    // 随机射击
    if (fireTimer_.IsFinished()) {
        tank.Fire();
        fireTimer_.Reset();
    }
}

void AIController::UpdateChase(Tank& tank, float dt) {
    auto* movement = tank.GetEntity()->GetComponent<MovementComponent>();
    if (!movement) return;

    Tank* target = FindNearestEnemy(tank);
    auto* myPos = tank.GetEntity()->GetComponent<TransformComponent>();
    if (!myPos) return;

    float dx, dy;

    if (target) {
        auto* tgtPos = target->GetEntity()->GetComponent<TransformComponent>();
        if (!tgtPos) { state_ = AIState::PATROL; return; }
        dx = tgtPos->GetPosition().x - myPos->GetPosition().x;
        dy = tgtPos->GetPosition().y - myPos->GetPosition().y;
    } else {
        // 没有敌方坦克 → 攻击敌方基地
        int enemyTeam = 1 - tank.GetTeam();
        Entity* base = bases_[enemyTeam];
        if (!base || !base->IsActive()) { state_ = AIState::PATROL; return; }
        auto* baseT = base->GetComponent<TransformComponent>();
        if (!baseT) { state_ = AIState::PATROL; return; }
        dx = baseT->GetPosition().x - myPos->GetPosition().x;
        dy = baseT->GetPosition().y - myPos->GetPosition().y;
    }

    // 朝目标方向移动
    if (std::abs(dx) > std::abs(dy)) {
        movement->SetDirection(dx > 0 ? Direction::RIGHT : Direction::LEFT);
    } else {
        movement->SetDirection(dy > 0 ? Direction::DOWN : Direction::UP);
    }
    movement->SetMoving(true);

    // 近距离 → 攻击
    float dist = std::sqrt(dx * dx + dy * dy);
    if (dist < detectRange_ * 0.6f) {
        state_ = AIState::ATTACK;
    }

    if (fireTimer_.IsFinished()) {
        tank.Fire();
        fireTimer_.Reset();
    }
}

void AIController::UpdateAttack(Tank& tank, float dt) {
    auto* movement = tank.GetEntity()->GetComponent<MovementComponent>();
    if (!movement) return;

    auto* myPos = tank.GetEntity()->GetComponent<TransformComponent>();
    if (!myPos) return;

    Tank* target = FindNearestEnemy(tank);
    float dx, dy;

    if (target) {
        auto* tgtPos = target->GetEntity()->GetComponent<TransformComponent>();
        if (!tgtPos) { state_ = AIState::PATROL; return; }
        dx = tgtPos->GetPosition().x - myPos->GetPosition().x;
        dy = tgtPos->GetPosition().y - myPos->GetPosition().y;
    } else {
        int enemyTeam = 1 - tank.GetTeam();
        Entity* base = bases_[enemyTeam];
        if (!base || !base->IsActive()) { state_ = AIState::PATROL; return; }
        auto* baseT = base->GetComponent<TransformComponent>();
        if (!baseT) { state_ = AIState::PATROL; return; }
        dx = baseT->GetPosition().x - myPos->GetPosition().x;
        dy = baseT->GetPosition().y - myPos->GetPosition().y;
    }

    // 对齐目标轴线后射击
    if (std::abs(dx) < TILE_SIZE * 0.5f) {
        movement->SetDirection(dy > 0 ? Direction::DOWN : Direction::UP);
    } else if (std::abs(dy) < TILE_SIZE * 0.5f) {
        movement->SetDirection(dx > 0 ? Direction::RIGHT : Direction::LEFT);
    }

    if (fireTimer_.IsFinished()) {
        tank.Fire();
        fireTimer_.Reset();
    }

    float dist = std::sqrt(dx * dx + dy * dy);
    if (dist > detectRange_) {
        state_ = AIState::PATROL;
    }
}

Tank* AIController::FindNearestEnemy(Tank& self) {
    if (!allTanks_) return nullptr;

    auto* myTransform = self.GetEntity()->GetComponent<TransformComponent>();
    if (!myTransform) return nullptr;

    Tank* nearest = nullptr;
    float minDist = detectRange_ * detectRange_;  // 用平方距离避免 sqrt

    for (auto* tank : *allTanks_) {
        if (!tank || tank == &self || tank->IsDead()) continue;
        if (tank->GetTeam() == self.GetTeam()) continue;  // 同队跳过

        auto* tgtTransform = tank->GetEntity()->GetComponent<TransformComponent>();
        if (!tgtTransform) continue;

        float dx = tgtTransform->GetPosition().x - myTransform->GetPosition().x;
        float dy = tgtTransform->GetPosition().y - myTransform->GetPosition().y;
        float dist2 = dx * dx + dy * dy;
        if (dist2 < minDist) {
            minDist = dist2;
            nearest = tank;
        }
    }
    return nearest;
}
