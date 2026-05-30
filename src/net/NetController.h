#pragma once

#include "game/tank/controller/IController.h"
#include "net/NetProtocol.h"
#include <cstdint>

class Tank;
class NetworkManager;

/// 网络控制器 (客户端用): 不读取本地键盘，等待网络输入
class NetController : public IController {
public:
    explicit NetController(int playerIndex);

    void Update(Tank& tank, float dt) override;
    bool IsAI() const override { return false; }

    /// Host 调用: 从网络消息解析输入并缓存
    void ApplyInput(uint8_t inputMask);

private:
    int playerIndex_;
    uint8_t currentInput_ = 0;  // 当前帧输入位掩码
};
