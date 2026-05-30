#pragma once

#include "ecs/Entity.h"
#include "game/map/TileType.h"
#include "core/Types.h"

/// 墙壁实体
class Wall : public Entity {
public:
    Wall(int id, TileType type, int gridCol, int gridRow);

    void Render() override;

    TileType GetTileType() const { return tileType_; }
    bool IsDestructible() const { return tileType_ == TileType::BRICK; }

    /// 被子弹击中
    bool OnHit();

    int GetGridCol() const { return gridCol_; }
    int GetGridRow() const { return gridRow_; }

private:
    TileType tileType_;
    int gridCol_, gridRow_;
    int hp_ = 1;
};
