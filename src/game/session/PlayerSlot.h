#pragma once

#include "game/tank/TankType.h"
#include "core/Types.h"
#include "core/Timer.h"
#include "raylib.h"

class Tank;

/// 玩家槽位：配置 + 运行时状态
struct PlayerSlot {
    bool     isHuman   = false;
    TankType tankType  = TankType::MEDIUM;
    int      team      = 0;     // 阵营 (0=红方, 1=蓝方)

    // 人类玩家键位
    int keyUp    = KEY_W;
    int keyDown  = KEY_S;
    int keyLeft  = KEY_A;
    int keyRight = KEY_D;
    int keyFire  = KEY_SPACE;

    // 运行时
    Tank* tank  = nullptr;
    int   score = 0;
    int   lives = DEFAULT_LIVES;

    // 攻防战重生系统
    float respawnTimer = 0.f;      // 重生倒计时
    bool  isRespawning = false;    // 是否正在等待重生
    int   kills = 0;               // 击杀数 (各自为战用)

    /// 获取显示用队伍颜色
    Color GetTeamColor() const { return (team == 0) ? RED : BLUE; }
};
