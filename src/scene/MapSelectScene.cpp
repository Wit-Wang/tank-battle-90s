#include "MapSelectScene.h"
#include "SceneManager.h"
#include "GameScene.h"
#include "SetupScene.h"
#include "game/map/GameMap.h"
#include "raylib.h"
#include <cmath>

MapSelectScene::MapSelectScene(SceneManager* manager, GameSession session)
    : manager_(manager), session_(std::move(session)),
      mode_(session_.GetGameMode()) {}

void MapSelectScene::Enter() {
    // 按游戏模式过滤地图
    mapManager_.ScanMapsForMode("assets/maps", mode_);
    animTimer_ = 0.f;
    transitioning_ = false;
    transProgress_ = 0.f;
    LoadPreview();
}

void MapSelectScene::LoadPreview() {
    const auto& info = mapManager_.GetCurrentMapInfo();
    preview_.LoadFromFile(info.filePath);
}

// 从 GameMap 快照瓦片数据 (用于过渡动画期间保留旧地图)
void MapSelectScene::StartTransition(int direction) {
    // 保存当前地图瓦片
    oldTiles_.resize(MAP_ROWS, std::vector<TileType>(MAP_COLS));
    for (int r = 0; r < MAP_ROWS; r++)
        for (int c = 0; c < MAP_COLS; c++)
            oldTiles_[r][c] = preview_.GetTile(c, r);

    // 切换地图 (direction 的视觉方向与逻辑相反:
    //  按 LEFT → 新地图从左侧滑入 → 逻辑上是 Next)
    if (direction < 0) mapManager_.Next();
    else               mapManager_.Previous();
    LoadPreview();

    // 保存新地图瓦片
    newTiles_.resize(MAP_ROWS, std::vector<TileType>(MAP_COLS));
    for (int r = 0; r < MAP_ROWS; r++)
        for (int c = 0; c < MAP_COLS; c++)
            newTiles_[r][c] = preview_.GetTile(c, r);

    // 启动动画
    transitioning_ = true;
    transDir_ = direction;
    transProgress_ = 0.f;
}

void MapSelectScene::Update(float dt) {
    animTimer_ += dt;

    // 过渡动画进行中 — 只更新动画，不接受输入
    if (transitioning_) {
        transProgress_ += TRANS_SPEED * dt;
        if (transProgress_ >= 1.f) {
            transProgress_ = 1.f;
            transitioning_ = false;
        }
        return;
    }

    // 只在动画结束 (或未开始) 时接受输入
    if (IsKeyPressed(KEY_LEFT))  StartTransition(-1);
    if (IsKeyPressed(KEY_RIGHT)) StartTransition(+1);

    if (IsKeyPressed(KEY_ENTER)) {
        session_.ApplyMapSelection(mapManager_);
        manager_->StartGame(std::make_unique<GameScene>(manager_, std::move(session_)));
    }
    if (IsKeyPressed(KEY_ESCAPE)) {
        manager_->SetupNext(std::make_unique<SetupScene>(manager_, std::move(session_)));
    }
}

// ---- 瓦片颜色辅助 ----
static Color TileToColor(TileType tile, float animTimer, int c, int r) {
    switch (tile) {
        case TileType::EMPTY: return {15, 15, 20, 255};
        case TileType::BRICK: return {160, 80, 40, 255};
        case TileType::STEEL: return {140, 145, 160, 255};
        case TileType::WATER: {
            float wave = std::sin(animTimer * 3.f + c * 0.8f + r * 0.5f);
            return {30, 80, 160, (unsigned char)(153 + 51 * wave)};
        }
        case TileType::GRASS: return {30, 100, 30, 255};
        case TileType::BASE:  return GOLD;
        default:              return BLACK;
    }
}

// ---- 绘制单张地图瓦片块 (快照版，用于过渡动画) ----
void MapSelectScene::DrawMapPreview(
    const std::vector<std::vector<TileType>>& tiles,
    int offsetX, int offsetY, int tileSize, float alpha)
{
    for (int r = 0; r < MAP_ROWS; r++) {
        for (int c = 0; c < MAP_COLS; c++) {
            if (r >= (int)tiles.size() || c >= (int)tiles[r].size()) continue;
            Color color = TileToColor(tiles[r][c], animTimer_, c, r);
            color.a = (unsigned char)(color.a * alpha);
            int px = offsetX + c * tileSize;
            int py = offsetY + r * tileSize;
            DrawRectangle(px, py, tileSize, tileSize, color);
            DrawRectangleLines(px, py, tileSize, tileSize,
                               ColorAlpha(DARKGRAY, 0.2f * alpha));
        }
    }
}

// ---- 绘制单张地图瓦片块 (GameMap 直接版，静态显示用) ----
void MapSelectScene::DrawMapPreview(
    const GameMap& map,
    int offsetX, int offsetY, int tileSize, float alpha)
{
    for (int r = 0; r < MAP_ROWS; r++) {
        for (int c = 0; c < MAP_COLS; c++) {
            Color color = TileToColor(map.GetTile(c, r), animTimer_, c, r);
            color.a = (unsigned char)(color.a * alpha);
            int px = offsetX + c * tileSize;
            int py = offsetY + r * tileSize;
            DrawRectangle(px, py, tileSize, tileSize, color);
            DrawRectangleLines(px, py, tileSize, tileSize,
                               ColorAlpha(DARKGRAY, 0.2f * alpha));
        }
    }
}

void MapSelectScene::Render() {
    ClearBackground(BLACK);

    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    // ---- 布局常量 ----
    constexpr int TILE_PX  = 24;                       // 24px/tile
    constexpr int MAP_PX_W = MAP_COLS * TILE_PX;       // 312
    constexpr int MAP_PX_H = MAP_ROWS * TILE_PX;       // 312
    int mapX = (sw - MAP_PX_W) / 2;                    // 水平居中
    int mapY = 120;

    // ---- 标题 ----
    const char* title = "SELECT MAP";
    DrawText(title, (sw - MeasureText(title, 32)) / 2, 20, 32, GREEN);

    // ---- 当前地图名称 + 难度 (动画期间显示新地图信息) ----
    const auto& info = mapManager_.GetCurrentMapInfo();
    int nameW = MeasureText(info.name.c_str(), 28);
    DrawText(info.name.c_str(), (sw - nameW) / 2, 62, 28, WHITE);

    char diffBuf[32];
    snprintf(diffBuf, sizeof(diffBuf), "Difficulty: %d/5", info.difficulty);
    int diffW = MeasureText(diffBuf, 14);
    DrawText(diffBuf, (sw - diffW) / 2, 95, 14, YELLOW);

    // ========================================================
    //  地图预览 — 过渡动画 / 静态显示
    // ========================================================
    if (transitioning_) {
        // ---- 平滑缓动 ----
        float t = transProgress_;
        float ease = t * t * (3.f - 2.f * t);  // smoothstep

        // 滑动距离 = 地图宽度 + 间距
        int slideDist = MAP_PX_W + 40;

        // old: 滑出 + 淡出
        int oldOffX = mapX + (int)(transDir_ * slideDist * ease);
        float oldAlpha = 1.f - ease;

        // new: 从对侧滑入 + 淡入
        int newOffX = mapX - transDir_ * (int)(slideDist * (1.f - ease));
        float newAlpha = ease;

        // 外框 — 跟随新地图位置
        float borderAlpha = 0.4f + 0.3f * std::sin(animTimer_ * 2.f);
        DrawRectangleLines(newOffX - 2, mapY - 2, MAP_PX_W + 4, MAP_PX_H + 4,
                           ColorAlpha(GREEN, borderAlpha * newAlpha));
        // 旧地图外框淡出
        DrawRectangleLines(oldOffX - 2, mapY - 2, MAP_PX_W + 4, MAP_PX_H + 4,
                           ColorAlpha(GREEN, borderAlpha * oldAlpha));

        // 绘制两张地图
        DrawMapPreview(oldTiles_, oldOffX, mapY, TILE_PX, oldAlpha);
        DrawMapPreview(newTiles_, newOffX, mapY, TILE_PX, newAlpha);
    } else {
        // ---- 静态：单张地图 ----
        float borderAlpha = 0.4f + 0.3f * std::sin(animTimer_ * 2.f);
        DrawRectangleLines(mapX - 2, mapY - 2, MAP_PX_W + 4, MAP_PX_H + 4,
                           ColorAlpha(GREEN, borderAlpha));
        DrawMapPreview(preview_, mapX, mapY, TILE_PX, 1.f);
    }

    // ========================================================
    //  队伍信息 (模式相关)
    // ========================================================
    int infoY = mapY + MAP_PX_H + 18;

    if (mode_ == GameMode::FREE_FOR_ALL) {
        const char* ffaInfo = "FREE FOR ALL — No teams, last standing wins";
        int ffaW = MeasureText(ffaInfo, 18);
        DrawText(ffaInfo, (sw - ffaW) / 2, infoY, 18, PURPLE);
    } else {
        int team0 = 0, team1 = 0;
        for (int i = 0; i < 4; i++) {
            if (session_.GetSlot(i).team == 0) team0++;
            else team1++;
        }
        char teamBuf[64];
        if (mode_ == GameMode::ATTACK_DEFEND) {
            snprintf(teamBuf, sizeof(teamBuf), "ATK x%d  vs  DEF x%d  (Defenders respawn)", team0, team1);
        } else {
            snprintf(teamBuf, sizeof(teamBuf), "ATK x%d  vs  DEF x%d", team0, team1);
        }
        int teamW = MeasureText(teamBuf, 20);
        DrawText(teamBuf, (sw - teamW) / 2, infoY, 20, WHITE);
    }
    infoY += 28;

    // 槽位 (一行 4 个，均匀分布)
    static Color ffaColors[] = { RED, BLUE, GREEN, PURPLE };
    for (int i = 0; i < 4; i++) {
        const auto& slot = session_.GetSlot(i);
        Color teamColor;
        if (mode_ == GameMode::FREE_FOR_ALL) {
            teamColor = ffaColors[i];
        } else {
            teamColor = slot.GetTeamColor();
        }
        const char* pType = slot.isHuman ? "P" : "AI";

        char buf[48];
        if (slot.isHuman) {
            const auto& stats = GetTankStats(slot.tankType);
            snprintf(buf, sizeof(buf), "%s%d %s", pType, i + 1, stats.name);
        } else {
            snprintf(buf, sizeof(buf), "%s%d RANDOM", pType, i + 1);
        }
        int bw = MeasureText(buf, 15);
        int bx = (sw - 4 * 120) / 2 + i * 120 + (120 - bw) / 2;
        DrawText(buf, bx, infoY, 15, teamColor);
    }

    // ---- 左右箭头提示 ----
    const char* arrowL = "<";
    const char* arrowR = ">";
    DrawText(arrowL, mapX - 30, mapY + MAP_PX_H / 2 - 10, 24, YELLOW);
    DrawText(arrowR, mapX + MAP_PX_W + 10, mapY + MAP_PX_H / 2 - 10, 24, YELLOW);

    // ---- 底部操作提示 ----
    const char* hint = "< > Switch   ENTER Start   ESC Back";
    float hintAlpha = 0.5f + 0.3f * std::sin(animTimer_ * 2.5f);
    DrawText(hint, (sw - MeasureText(hint, 16)) / 2, sh - 35, 16,
             ColorAlpha(YELLOW, hintAlpha));
}
