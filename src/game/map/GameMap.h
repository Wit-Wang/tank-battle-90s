#pragma once

#include "TileType.h"
#include "core/Types.h"
#ifdef TANKGAME_SERVER
#include "utils/raylib_stubs.h"
#else
#include "raylib.h"
#endif
#include <vector>
#include <string>

/// 地图信息 (元数据)
struct MapInfo {
    std::string name;
    std::string filePath;
    int difficulty = 1;  // 1-5
    GameMode mode = GameMode::TRADITIONAL;  // 适用的游戏模式
};

/// 地图数据 + 渲染：管理 13×13 瓦片网格
class GameMap {
public:
    GameMap() = default;

    /// 从 JSON 文件加载
    bool LoadFromFile(const std::string& path);

    /// 渲染地图瓦片
    void Render();

    /// 获取瓦片
    TileType GetTile(int col, int row) const;

    /// 设置瓦片 (用于破坏墙壁、加固基地)
    void SetTile(int col, int row, TileType type);

    /// 获取出生点 (4 个)
    const std::vector<Vector2>& GetSpawnPoints() const { return spawnPoints_; }

    /// 获取基地位置 (单基地，兼容)
    Vector2 GetBasePosition() const { return basePosition_; }

    /// 获取队伍基地位置 (0=攻方, 1=守方)
    Vector2 GetTeamBasePosition(int team) const { return teamBasePositions_[team]; }

    /// 获取队伍出生点 (每队 2 个)
    const std::vector<Vector2>& GetTeamSpawnPoints(int team) const { return teamSpawnPoints_[team]; }

    /// 获取地图尺寸
    int GetWidth()  const { return MAP_COLS; }
    int GetHeight() const { return MAP_ROWS; }

    /// 瓦片是否可通行
    bool IsWalkable(int col, int row) const;

    /// 瓦片是否阻挡子弹
    bool BlocksBullet(int col, int row) const;

    /// 获取瓦片颜色 (临时渲染方案，无纹理时使用)
    static Color GetTileColor(TileType type);

private:
    std::vector<std::vector<TileType>> tiles_;
    std::vector<Vector2> spawnPoints_;
    Vector2 basePosition_{ 0, 0 };
    Vector2 teamBasePositions_[2]{};
    std::vector<Vector2> teamSpawnPoints_[2];
};
