#pragma once

#include "Component.h"
#include <vector>
#include <memory>
#include <typeindex>
#include <algorithm>

/// 实体基类：拥有组件列表，通过组合实现行为
class Entity {
public:
    explicit Entity(int id = -1) : id_(id), active_(true) {}
    virtual ~Entity() = default;

    virtual void Update(float dt);
    virtual void Render();

    /// 添加组件
    template<typename T, typename... Args>
    T* AddComponent(Args&&... args) {
        auto comp = std::make_unique<T>(std::forward<Args>(args)...);
        comp->SetOwner(this);
        T* ptr = comp.get();
        components_.push_back(std::move(comp));
        return ptr;
    }

    /// 获取组件
    template<typename T>
    T* GetComponent() const {
        for (auto& c : components_) {
            if (auto* ptr = dynamic_cast<T*>(c.get())) {
                return ptr;
            }
        }
        return nullptr;
    }

    /// 移除组件
    template<typename T>
    void RemoveComponent() {
        std::erase_if(components_, [](const std::unique_ptr<Component>& c) {
            return dynamic_cast<T*>(c.get()) != nullptr;
        });
    }

    int  GetId()     const { return id_; }
    bool IsActive()  const { return active_; }
    void SetActive(bool v) { active_ = v; }

protected:
    int id_;
    bool active_;
    std::vector<std::unique_ptr<Component>> components_;
};
