#pragma once

#include "ecs/Component.h"
#include "core/Types.h"
#ifdef TANKGAME_SERVER
#include "utils/raylib_stubs.h"
#else
#include "raylib.h"
#endif

/// 精灵渲染组件：绑定纹理、着色、翻转
class SpriteComponent : public Component {
public:
    SpriteComponent() = default;
    explicit SpriteComponent(Texture2D* tex, Color tint = WHITE)
        : texture(tex), tint(tint) {}

    void Render() override;

    Texture2D* texture = nullptr;
    Color      tint    = WHITE;
    bool       flipX   = false;
    bool       flipY   = false;
    /// 源矩形 (用于 sprite sheet 切片)
    Rectangle  sourceRect{ 0, 0, 0, 0 };
    bool       useSourceRect = false;
};
