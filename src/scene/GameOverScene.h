#pragma once

#include "Scene.h"
#include "game/session/GameSession.h"
#include "core/Types.h"

class SceneManager;

/// 游戏结束场景：模式相关结果展示
class GameOverScene : public Scene {
public:
    /// @param winningTeam 获胜队伍 (FFA 模式下为获胜玩家索引)
    GameOverScene(SceneManager* manager, int winningTeam, GameSession session);

    void Update(float dt) override;
    void Render() override;

private:
    SceneManager* manager_;
    GameSession session_;
    int winningTeam_;
    GameMode mode_;
    float timer_ = 0.f;
    bool inputReady_ = false;
};
