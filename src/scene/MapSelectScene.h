#pragma once

#include "Scene.h"
#include "game/session/GameSession.h"
#include "game/map/MapManager.h"
#include "game/map/GameMap.h"
#include "game/map/TileType.h"
#include "core/Types.h"
#include <vector>

class SceneManager;

/// 地图选择：按游戏模式过滤地图，预览并选择
class MapSelectScene : public Scene {
public:
    MapSelectScene(SceneManager* manager, GameSession session);

    void Enter() override;
    void Update(float dt) override;
    void Render() override;

private:
    void LoadPreview();
    void StartTransition(int direction);  // -1 = left, +1 = right
    void DrawMapPreview(const std::vector<std::vector<TileType>>& tiles,
                        int offsetX, int offsetY, int tileSize, float alpha);
    void DrawMapPreview(const GameMap& map,
                        int offsetX, int offsetY, int tileSize, float alpha);

    SceneManager* manager_;
    GameSession session_;
    GameMode mode_;
    MapManager mapManager_;
    GameMap preview_;
    float animTimer_ = 0.f;

    // ---- 切换动画状态 ----
    bool  transitioning_ = false;
    int   transDir_      = 0;       // -1 向左换, +1 向右换
    float transProgress_ = 0.f;     // 0→1
    static constexpr float TRANS_SPEED = 3.5f;  // 动画速度 (越大越快)
    std::vector<std::vector<TileType>> oldTiles_;  // 切换前的地图快照
    std::vector<std::vector<TileType>> newTiles_;  // 切换后的地图快照
};
