#pragma once

#ifdef TANKGAME_SERVER
#include "utils/raylib_stubs.h"
#else
#include "raylib.h"
#endif

/// 统一输入查询，封装 raylib 键盘/手柄输入
class InputManager {
public:
    static bool IsKeyDown(int key);
    static bool IsKeyPressed(int key);
    static bool IsKeyUp(int key);
};
