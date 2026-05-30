#pragma once

#include "Scene.h"
#include "core/Types.h"

class SceneManager;

/// 游戏模式选择场景：传统对战 / 攻防战 / 各自为战
class ModeSelectScene : public Scene {
public:
    explicit ModeSelectScene(SceneManager* manager) : manager_(manager) {}

    void Enter() override;
    void Update(float dt) override;
    void Render() override;

private:
    void RenderModeCard(int index, int y);

    SceneManager* manager_;
    int selection_ = 0;        // 0=Traditional, 1=Attack/Defend, 2=FFA
    float animTimer_ = 0.f;
};
