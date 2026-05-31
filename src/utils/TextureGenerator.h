#pragma once

#include "raylib.h"
#include "game/tank/TankType.h"
#include "game/map/TileType.h"

/// 程序化纹理生成器：CPU 上逐像素绘制，不依赖外部文件
class TextureGenerator {
public:
    /// 生成所有游戏纹理并注册到 ResourceManager
    static void GenerateAll();

    /// 生成坦克纹理 (64x64, team: 0=红 1=蓝 2=绿 3=紫, 面朝上方)
    static Texture2D GenerateTank(TankType type, int team);

    /// 生成子弹纹理 (8x8, team: 0=红 1=蓝 2=绿 3=紫)
    static Texture2D GenerateBullet(int team);

    /// 生成地砖纹理 (64x64)
    static Texture2D GenerateTile(TileType type);

    /// 生成基地纹理 (64x64, team: 0=红 1=蓝 2=绿 3=紫)
    static Texture2D GenerateBase(int team, bool destroyed);
};
