#pragma once

#include "Scene.h"
#include "game/session/GameSession.h"
#include "core/Types.h"

class SceneManager;

/// 配置场景：合并原 Lobby + CharacterSelect + TeamSelect
/// 在同一界面完成：人/AI 切换 + 坦克类型选择 + 队伍分配
class SetupScene : public Scene {
public:
    SetupScene(SceneManager* manager, GameMode mode);
    SetupScene(SceneManager* manager, GameSession session);

    void Enter() override;
    void Update(float dt) override;
    void Render() override;

private:
    void RenderSlot(int index);

    SceneManager* manager_;
    GameMode mode_;
    GameSession session_;
    int currentSlot_ = 0;
    bool hasSession_ = false;
};
