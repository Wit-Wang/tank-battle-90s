#pragma once

#include "Scene.h"

class SceneManager;

/// 暂停覆盖层：覆盖在 GameScene 之上
class PauseScene : public Scene {
public:
    explicit PauseScene(SceneManager* manager) : manager_(manager) {}

    void Update(float dt) override;
    void Render() override;
    bool IsOverlay() const override { return true; }

private:
    SceneManager* manager_;
    float timer_ = 0.f;
};
