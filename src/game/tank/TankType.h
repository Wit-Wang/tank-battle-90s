#pragma once

#include <unordered_map>

/// 坦克类型枚举
enum class TankType {
    LIGHT,   // 轻型：速度快，血薄，射速快
    MEDIUM,  // 中型：均衡
    HEAVY,   // 重型：慢，血厚，高护甲
    SPEED,   // 极速：极快，脆皮，三连发
};

/// 坦克属性表 (属性表驱动，非继承)
struct TankStats {
    float moveSpeed;       // 移动速度 (像素/秒)
    float bulletSpeed;     // 子弹速度
    int   bulletDamage;    // 子弹伤害
    int   maxHealth;       // 最大生命值
    int   armor;           // 护甲 (减伤)
    float fireCooldown;    // 射击冷却 (秒)
    int   maxBullets;      // 同时最多子弹数
    const char* name;      // 显示名称
    const char* spritePath; // 精灵图路径
};

/// 预定义属性表 — 加新坦克只需加一行
/// 设计原则:
///   子弹速度 >= 最快坦克速度 x3 (确保子弹能追上所有坦克)
///   血量: LIGHT=1, MEDIUM=2, HEAVY=3, SPEED=1
///   护甲: LIGHT=0, MEDIUM=1, HEAVY=2, SPEED=0
///   有效伤害 = max(1, bulletDamage - armor)
inline const std::unordered_map<TankType, TankStats> TANK_STATS = {
    { TankType::LIGHT,  { 100.f, 350.f, 1, 1, 0, 0.5f, 2, "LIGHT",  "assets/textures/tank_light.png"  }},
    { TankType::MEDIUM, { 80.f,  300.f, 2, 2, 1, 0.7f, 1, "MEDIUM", "assets/textures/tank_medium.png" }},
    { TankType::HEAVY,  { 55.f,  250.f, 2, 3, 2, 1.0f, 1, "HEAVY",  "assets/textures/tank_heavy.png"  }},
    { TankType::SPEED,  { 130.f, 400.f, 1, 1, 0, 0.3f, 3, "SPEED",  "assets/textures/tank_speed.png"  }},
};

inline const TankStats& GetTankStats(TankType type) {
    return TANK_STATS.at(type);
}
