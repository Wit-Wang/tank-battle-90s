#pragma once

#include "Entity.h"
#include <vector>
#include <memory>
#include <functional>

/// 实体生命周期管理器：创建、更新、渲染、销毁实体
class EntityManager {
public:
    EntityManager() = default;

    /// 创建实体并返回指针
    template<typename T, typename... Args>
    T* CreateEntity(Args&&... args) {
        auto entity = std::make_unique<T>(nextId_++, std::forward<Args>(args)...);
        T* ptr = entity.get();
        entities_.push_back(std::move(entity));
        return ptr;
    }

    /// 更新所有活跃实体
    void Update(float dt);

    /// 渲染所有活跃实体
    void Render();

    /// 清除所有标记为不活跃的实体
    void RemoveInactive();

    /// 清除所有实体
    void Clear();

    /// 遍历实体
    void ForEach(const std::function<void(Entity*)>& func);

    /// 获取所有实体
    const std::vector<std::unique_ptr<Entity>>& GetEntities() const { return entities_; }

private:
    std::vector<std::unique_ptr<Entity>> entities_;
    int nextId_ = 0;
};
