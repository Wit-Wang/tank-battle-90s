#pragma once

#include "raylib.h"
#include "Types.h"

/// 封装 raylib 窗口的创建与销毁
class Window {
public:
    Window();
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool ShouldClose() const;
    void BeginFrame();
    void EndFrame();

    int  GetWidth()  const { return SCREEN_WIDTH; }
    int  GetHeight() const { return SCREEN_HEIGHT; }
};
