#include "SpriteComponent.h"
#include "ecs/Entity.h"
#include "TransformComponent.h"

void SpriteComponent::Render() {
    if (!texture) return;

    auto* transform = GetOwner()->GetComponent<TransformComponent>();
    if (!transform) return;

    Vector2 pos = transform->GetPosition();
    float   rot = transform->GetRotation();
    float   scl = transform->GetScale();

    Rectangle src;
    if (useSourceRect) {
        src = sourceRect;
        if (flipX) src.width  = -src.width;
        if (flipY) src.height = -src.height;
    } else {
        src = { 0.f, 0.f,
                static_cast<float>(texture->width),
                static_cast<float>(texture->height) };
        if (flipX) src.width  = -src.width;
        if (flipY) src.height = -src.height;
    }

    Rectangle dst = {
        pos.x, pos.y,
        std::abs(src.width) * scl,
        std::abs(src.height) * scl
    };

    Vector2 origin = { dst.width / 2.f, dst.height / 2.f };

    DrawTexturePro(*texture, src, dst, origin, rot, tint);
}
