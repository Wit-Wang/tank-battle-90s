// winsock2.h 必须在 windows.h (raylib) 之前包含
#include "net/NetController.h"
#include "game/tank/Tank.h"
#include "ecs/components/MovementComponent.h"
#include "core/Types.h"

NetController::NetController(int playerIndex)
    : playerIndex_(playerIndex) {}

void NetController::Update(Tank& tank, float dt) {
    auto* movement = tank.GetEntity()->GetComponent<MovementComponent>();
    if (!movement) return;

    movement->SetMoving(false);

    if (currentInput_ & NET_INPUT_UP) {
        movement->SetDirection(Direction::UP);
        movement->SetMoving(true);
    } else if (currentInput_ & NET_INPUT_DOWN) {
        movement->SetDirection(Direction::DOWN);
        movement->SetMoving(true);
    } else if (currentInput_ & NET_INPUT_LEFT) {
        movement->SetDirection(Direction::LEFT);
        movement->SetMoving(true);
    } else if (currentInput_ & NET_INPUT_RIGHT) {
        movement->SetDirection(Direction::RIGHT);
        movement->SetMoving(true);
    }

    if (currentInput_ & NET_INPUT_FIRE) {
        tank.Fire();
    }

    // 每帧消耗输入 (单次触发的 fire 需要客户端持续发送)
    // currentInput_ 由 Host 每帧通过 ApplyInput 更新
}

void NetController::ApplyInput(uint8_t inputMask) {
    currentInput_ = inputMask;
}
