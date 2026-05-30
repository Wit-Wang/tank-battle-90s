#include "GameMap.h"
#include "raylib.h"
#include "core/ResourceManager.h"
#include <fstream>
#include <sstream>

bool GameMap::LoadFromFile(const std::string& path) {
    // 简化版：使用文本格式加载 (后续可改为 JSON)
    // 格式：每行 13 个数字 (0-5)，空格分隔
    std::ifstream file(path);
    if (!file.is_open()) return false;

    tiles_.clear();
    tiles_.resize(MAP_ROWS, std::vector<TileType>(MAP_COLS, TileType::EMPTY));

    std::string line;
    int row = 0;
    while (std::getline(file, line) && row < MAP_ROWS) {
        std::istringstream iss(line);
        int val;
        int col = 0;
        while (iss >> val && col < MAP_COLS) {
            tiles_[row][col] = static_cast<TileType>(val);
            col++;
        }
        row++;
    }

    // 从地图瓦片中收集基地位置 (TileType::BASE = 5)
    // 收集后清除 BASE 瓦片为 EMPTY，避免阻挡子弹
    // (基地作为 Entity 存在，由碰撞系统处理)
    std::vector<Vector2> basePositions;
    for (int r = 0; r < MAP_ROWS; r++) {
        for (int c = 0; c < MAP_COLS; c++) {
            if (tiles_[r][c] == TileType::BASE) {
                basePositions.push_back({
                    c * TILE_SIZE + TILE_SIZE / 2.f,
                    r * TILE_SIZE + TILE_SIZE / 2.f
                });
                tiles_[r][c] = TileType::EMPTY;  // 清除瓦片，由 Entity 接管
            }
        }
    }

    // 根据基地数量设置基地位置
    if (basePositions.size() >= 2) {
        // 传统对战: 2个基地 (第一个=team0, 第二个=team1)
        teamBasePositions_[0] = basePositions[0];
        teamBasePositions_[1] = basePositions[1];
        basePosition_ = basePositions[0];  // 兼容
    } else if (basePositions.size() == 1) {
        // 攻防战: 1个基地 (守方=team1)
        teamBasePositions_[1] = basePositions[0];
        teamBasePositions_[0] = basePositions[0];  // 占位
        basePosition_ = basePositions[0];
    } else {
        // 各自为战/无基地
        basePosition_ = { TILE_SIZE * 6 + TILE_SIZE / 2.f, TILE_SIZE * (MAP_ROWS - 1) + TILE_SIZE / 2.f };
        teamBasePositions_[0] = { TILE_SIZE * 6 + TILE_SIZE / 2.f, TILE_SIZE / 2.f };
        teamBasePositions_[1] = { TILE_SIZE * 6 + TILE_SIZE / 2.f, TILE_SIZE * (MAP_ROWS - 1) + TILE_SIZE / 2.f };
    }

    // 出生点: 4角 (所有模式通用)
    spawnPoints_ = {
        { TILE_SIZE * 1.f + TILE_SIZE / 2, TILE_SIZE / 2.f },
        { TILE_SIZE * (MAP_COLS - 2) + TILE_SIZE / 2, TILE_SIZE / 2.f },
        { TILE_SIZE / 2.f, TILE_SIZE * (MAP_ROWS - 1) + TILE_SIZE / 2 },
        { TILE_SIZE * (MAP_COLS - 1) + TILE_SIZE / 2, TILE_SIZE * (MAP_ROWS - 1) + TILE_SIZE / 2 },
    };

    // 队伍出生点: 每队 2 个
    teamSpawnPoints_[0] = {
        { TILE_SIZE * 1.f + TILE_SIZE / 2, TILE_SIZE / 2.f },
        { TILE_SIZE * (MAP_COLS - 2) + TILE_SIZE / 2, TILE_SIZE / 2.f },
    };
    teamSpawnPoints_[1] = {
        { TILE_SIZE * 1.f + TILE_SIZE / 2, TILE_SIZE * (MAP_ROWS - 1) + TILE_SIZE / 2 },
        { TILE_SIZE * (MAP_COLS - 1) + TILE_SIZE / 2, TILE_SIZE * (MAP_ROWS - 1) + TILE_SIZE / 2 },
    };

    return true;
}

void GameMap::Render() {
    static const char* tileNames[] = { "tile_empty", "tile_brick", "tile_steel", "tile_water", "tile_grass", "tile_base" };
    auto& rm = ResourceManager::Instance();

    for (int r = 0; r < MAP_ROWS; r++) {
        for (int c = 0; c < MAP_COLS; c++) {
            TileType type = tiles_[r][c];
            if (type == TileType::EMPTY) continue;

            float x = c * TILE_SIZE;
            float y = r * TILE_SIZE;

            // 尝试使用纹理
            int idx = static_cast<int>(type);
            if (idx >= 0 && idx < 6) {
                const std::string& name = tileNames[idx];
                if (rm.HasTexture(name)) {
                    Texture2D& tex = rm.GetTexture(name);
                    DrawTexture(tex, static_cast<int>(x), static_cast<int>(y), WHITE);
                    continue;
                }
            }

            // 回退到颜色矩形
            DrawRectangleV({ x, y }, { TILE_SIZE, TILE_SIZE }, GetTileColor(type));
            if (type == TileType::BRICK) {
                DrawRectangleLines(x, y, TILE_SIZE, TILE_SIZE, DARKBROWN);
            }
        }
    }
}

TileType GameMap::GetTile(int col, int row) const {
    if (col < 0 || col >= MAP_COLS || row < 0 || row >= MAP_ROWS)
        return TileType::STEEL;  // 边界外视为钢墙
    return tiles_[row][col];
}

void GameMap::SetTile(int col, int row, TileType type) {
    if (col >= 0 && col < MAP_COLS && row >= 0 && row < MAP_ROWS) {
        tiles_[row][col] = type;
    }
}

bool GameMap::IsWalkable(int col, int row) const {
    TileType t = GetTile(col, row);
    return t == TileType::EMPTY || t == TileType::GRASS || t == TileType::BASE;
}

bool GameMap::BlocksBullet(int col, int row) const {
    TileType t = GetTile(col, row);
    return t == TileType::BRICK || t == TileType::STEEL || t == TileType::BASE;
}

Color GameMap::GetTileColor(TileType type) {
    switch (type) {
        case TileType::BRICK: return BROWN;
        case TileType::STEEL: return LIGHTGRAY;
        case TileType::WATER: return BLUE;
        case TileType::GRASS: return DARKGREEN;
        case TileType::BASE:  return GOLD;
        default:              return BLACK;
    }
}
