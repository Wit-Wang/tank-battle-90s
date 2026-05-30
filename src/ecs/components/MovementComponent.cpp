#include "MovementComponent.h"
#include "ecs/Entity.h"
#include "TransformComponent.h"
#include "utils/MathUtils.h"

void MovementComponent::Update(float dt) {
    if (!moving_) return;

    auto* transform = GetOwner()->GetComponent<TransformComponent>();
    if (!transform) return;

    transform->SetPosition(TryMove(transform->GetPosition(), dt));
    transform->SetRotation(DirectionToAngle(direction_));
}

Vector2 MovementComponent::TryMove(Vector2 current, float dt) const {
    Vector2 dir = DirectionToVector(direction_);
    return {
        current.x + dir.x * speed_ * dt,
        current.y + dir.y * speed_ * dt
    };
}
