#pragma once

#include "Scene.h"
#include "game/session/GameSession.h"
#include "core/Types.h"

class SceneManager;

/// 队伍选择：为每个槽位分配队伍 (模式决定可编辑性)
class TeamSelectScene : public Scene {
public:
    TeamSelectScene(SceneManager* manager, GameSession session);

    void Enter() override;
    void Update(float dt) override;
    void Render() override;

private:
    SceneManager* manager_;
    GameSession session_;
    GameMode mode_;
    int currentSlot_ = 0;
};
