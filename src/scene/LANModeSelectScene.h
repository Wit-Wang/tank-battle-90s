#pragma once

#include "Scene.h"

class SceneManager;

/// LAN 模式选择: Host Game / Join Game / Back
class LANModeSelectScene : public Scene {
public:
    explicit LANModeSelectScene(SceneManager* manager);

    void Enter() override;
    void Update(float dt) override;
    void Render() override;

private:
    SceneManager* manager_;
    int selection_ = 0;  // 0=Host, 1=Join, 2=Back
};
