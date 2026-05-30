#pragma once

/// 场景抽象基类 (状态模式)
class Scene {
public:
    virtual ~Scene() = default;

    /// 进入场景时调用
    virtual void Enter() {}

    /// 离开场景时调用
    virtual void Exit() {}

    /// 每帧更新
    virtual void Update(float dt) = 0;

    /// 每帧渲染
    virtual void Render() = 0;

    /// 是否需要下层场景继续渲染 (用于暂停覆盖层)
    virtual bool IsOverlay() const { return false; }
};
