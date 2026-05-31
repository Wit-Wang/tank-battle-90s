#pragma once

#ifdef TANKGAME_SERVER
#include "utils/raylib_stubs.h"
#else
#include "raylib.h"
#endif
#include <cstdint>
#include <string>

// ---- Window ----
constexpr int SCREEN_WIDTH  = 832;   // 13 * 64
constexpr int SCREEN_HEIGHT = 832;
constexpr int TILE_SIZE     = 64;
constexpr const char* GAME_TITLE = "Tank Battle 90s";

// ---- Grid ----
constexpr int MAP_COLS = 13;
constexpr int MAP_ROWS = 13;

// ---- Gameplay ----
constexpr int   DEFAULT_LIVES = 3;
constexpr int   MAX_PLAYERS   = 4;
constexpr int   ATTACKER_LIVES = 3;       // 攻防战: 攻方命�
constexpr int   DEFENDER_LIVES = -1;      // 攻防战: 守方无限 (-1)
constexpr float RESPAWN_DELAY  = 3.0f;    // 攻防战: 守方重生延迟 (秒)
constexpr float RESPAWN_INVINCIBLE = 2.0f; // 重生后无敌时间

// ---- Direction ----
enum class Direction { UP, DOWN, LEFT, RIGHT };

// ---- Game Mode ----
enum class GameMode {
    TRADITIONAL,     // 传统对战: 2队, 各有基地, 无重生
    ATTACK_DEFEND,   // 攻防战: 攻方(有限命)vs守方(无限重生+延迟)
    FREE_FOR_ALL,    // 各自为战: 4人混战, 无队伍无基地
};
