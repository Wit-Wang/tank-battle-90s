#pragma once

class Entity;

/// 组件抽象基类 (组件模式)
/// 每个组件附加到一个 Entity，通过虚函数实现行为组合
class Component {
public:
    virtual ~Component() = default;

    virtual void Update(float dt) {}
    virtual void Render() {}

    void SetOwner(Entity* owner) { owner_ = owner; }
    Entity* GetOwner() const { return owner_; }

protected:
    Entity* owner_ = nullptr;
};
