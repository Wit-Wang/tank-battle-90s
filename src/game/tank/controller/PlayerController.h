#pragma once

#include "IController.h"

/// 人类玩家控制器：通过键盘输入控制坦克
class PlayerController : public IController {
public:
    PlayerController(int keyUp, int keyDown, int keyLeft, int keyRight, int keyFire);

    void Update(Tank& tank, float dt) override;
    bool IsAI() const override { return false; }

private:
    int keyUp_, keyDown_, keyLeft_, keyRight_, keyFire_;
};
