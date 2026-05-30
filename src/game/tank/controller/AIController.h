#pragma once

#include "IController.h"
#include "core/Timer.h"
#include "core/Types.h"
#include "ecs/Entity.h"
#include <vector>

class Tank;

/// AI 控制器：自主行为 (巡逻 → 追击 → 射击)
class AIController : public IController {
public:
    AIController();

    void Update(Tank& tank, float dt) override;
    bool IsAI() const override { return true; }

    /// 设置所有坦克的引用 (用于查找敌人)
    static void SetAllTanks(const std::vector<Tank*>* tanks) { allTanks_ = tanks; }

    /// 设置双方基地引用 (AI 攻击目标)
    static void SetBases(Entity* base0, Entity* base1) { bases_[0] = base0; bases_[1] = base1; }

private:
    enum class AIState { PATROL, CHASE, ATTACK };

    void UpdatePatrol(Tank& tank, float dt);
    void UpdateChase(Tank& tank, float dt);
    void UpdateAttack(Tank& tank, float dt);

    /// 寻找最近的敌方坦克
    Tank* FindNearestEnemy(Tank& self);

    AIState state_ = AIState::PATROL;
    Direction patrolDir_ = Direction::UP;
    Timer dirChangeTimer_{ 2.0f };
    Timer fireTimer_{ 0.8f };
    float detectRange_ = 300.f;

    static const std::vector<Tank*>* allTanks_;
    static Entity* bases_[2];
};
