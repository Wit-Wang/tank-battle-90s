#pragma once

#include "core/Types.h"

class GameSession;

/// 游戏内 HUD：模式相关的游戏状态显示
class HUD {
public:
    explicit HUD(const GameSession& session);

    void Render();

private:
    void RenderTraditional();
    void RenderAttackDefend();
    void RenderFreeForAll();

    const GameSession& session_;
    GameMode mode_;
};
