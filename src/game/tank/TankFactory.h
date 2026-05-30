#pragma once

#include "Tank.h"
#include <memory>

/// 坦克工厂 (工厂模式)：根据 TankType + Controller 创建完整坦克
class TankFactory {
public:
    /// 创建玩家坦克
    static std::unique_ptr<Tank> CreatePlayerTank(
        TankType type, int team,
        int keyUp, int keyDown, int keyLeft, int keyRight, int keyFire);

    /// 创建 AI 坦克
    static std::unique_ptr<Tank> CreateAITank(TankType type, int team);
};
