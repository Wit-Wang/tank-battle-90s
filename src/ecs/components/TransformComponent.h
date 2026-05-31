#pragma once

#include "ecs/Component.h"
#include "core/Types.h"

/// 变换组件：位置、旋转、缩放
class TransformComponent : public Component {
public:
    TransformComponent() = default;
    TransformComponent(Vector2 pos, float rot = 0.f, float s = 1.f)
        : position_(pos), rotation_(rot), scale_(s) {}

    void Update(float dt) override {}

    // ---- Getters ----
    Vector2 GetPosition() const { return position_; }
    Vector2 GetPrevPosition() const { return prevPosition_; }
    float   GetRotation() const { return rotation_; }
    float   GetScale()    const { return scale_; }

    // ---- Setters ----
    void SetPosition(Vector2 p) { position_ = p; }
    void SetRotation(float r)   { rotation_ = r; }
    void SetScale(float s)      { scale_ = s; }

    /// 在移动前调用，保存当前位置用于碰撞系统判断移动轴
    void SavePosition() { prevPosition_ = position_; }

    // ---- 便捷操作 ----
    void Translate(Vector2 delta) {
        position_.x += delta.x;
        position_.y += delta.y;
    }

    /// 获取该实体所在的网格坐标 (col, row)
    void GetGridCell(int& col, int& row) const {
        col = static_cast<int>(position_.x) / TILE_SIZE;
        row = static_cast<int>(position_.y) / TILE_SIZE;
    }

private:
    Vector2 position_{ 0.f, 0.f };
    Vector2 prevPosition_{ 0.f, 0.f };
    float   rotation_ = 0.f;
    float   scale_    = 1.f;
};
