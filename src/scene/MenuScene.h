#pragma once

#include "Scene.h"

class SceneManager;

/// 主菜单场景：标题画面，选择热座或联机模式
class MenuScene : public Scene {
public:
    explicit MenuScene(SceneManager* manager) : manager_(manager) {}

    void Enter() override;
    void Update(float dt) override;
    void Render() override;

private:
    SceneManager* manager_;
    float titleBlink_ = 0.f;
    bool modeSelecting_ = false;   // true = 选择模式子菜单
    int modeSelection_ = 0;        // 0=Hot Seat, 1=LAN, 2=Back
};
