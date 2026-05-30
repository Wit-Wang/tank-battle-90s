#include "EntityManager.h"

void EntityManager::Update(float dt) {
    // Index-based iteration — safe if vector grows during update
    size_t count = entities_.size();
    for (size_t i = 0; i < count; i++) {
        if (entities_[i] && entities_[i]->IsActive()) {
            entities_[i]->Update(dt);
        }
    }
}

void EntityManager::Render() {
    size_t count = entities_.size();
    for (size_t i = 0; i < count; i++) {
        if (entities_[i] && entities_[i]->IsActive()) {
            entities_[i]->Render();
        }
    }
}

void EntityManager::RemoveInactive() {
    std::erase_if(entities_, [](const std::unique_ptr<Entity>& e) {
        return !e || !e->IsActive();
    });
}

void EntityManager::Clear() {
    entities_.clear();
}

void EntityManager::ForEach(const std::function<void(Entity*)>& func) {
    // Index-based — safe if vector grows during callback (e.g. entity creation)
    size_t count = entities_.size();
    for (size_t i = 0; i < count; i++) {
        if (entities_[i] && entities_[i]->IsActive()) {
            func(entities_[i].get());
        }
    }
}
