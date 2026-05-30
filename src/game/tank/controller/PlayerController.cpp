#include "PlayerController.h"
#include "game/tank/Tank.h"
#include "core/InputManager.h"
#include "ecs/components/MovementComponent.h"

PlayerController::PlayerController(int keyUp, int keyDown, int keyLeft, int keyRight, int keyFire)
    : keyUp_(keyUp), keyDown_(keyDown), keyLeft_(keyLeft), keyRight_(keyRight), keyFire_(keyFire) {}

void PlayerController::Update(Tank& tank, float dt) {
    auto* movement = tank.GetEntity()->GetComponent<MovementComponent>();
    if (!movement) return;

    movement->SetMoving(false);

    if (InputManager::IsKeyDown(keyUp_)) {
        movement->SetDirection(Direction::UP);
        movement->SetMoving(true);
    } else if (InputManager::IsKeyDown(keyDown_)) {
        movement->SetDirection(Direction::DOWN);
        movement->SetMoving(true);
    } else if (InputManager::IsKeyDown(keyLeft_)) {
        movement->SetDirection(Direction::LEFT);
        movement->SetMoving(true);
    } else if (InputManager::IsKeyDown(keyRight_)) {
        movement->SetDirection(Direction::RIGHT);
        movement->SetMoving(true);
    }

    if (InputManager::IsKeyPressed(keyFire_)) {
        tank.Fire();
    }
}
