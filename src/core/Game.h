#pragma once

#include "Window.h"
#include <memory>

class SceneManager;

/// 游戏主类：持有窗口、场景管理器，驱动主循环
class Game {
public:
    Game();
    ~Game();

    void Run();

private:
    void Init();
    void Shutdown();

    Window window_;
    std::unique_ptr<SceneManager> sceneManager_;
};
