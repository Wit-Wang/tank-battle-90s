#pragma once

class Tank;

/// 控制器接口 (策略模式)
/// 分离坦克的控制逻辑：人类输入 vs AI 行为
class IController {
public:
    virtual ~IController() = default;

    /// 每帧更新，驱动坦克行为
    virtual void Update(Tank& tank, float dt) = 0;

    /// 是否为 AI 控制
    virtual bool IsAI() const = 0;
};
