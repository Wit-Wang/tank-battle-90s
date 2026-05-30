#include "Entity.h"

void Entity::Update(float dt) {
    if (!active_) return;
    for (auto& c : components_) {
        c->Update(dt);
    }
}

void Entity::Render() {
    if (!active_) return;
    for (auto& c : components_) {
        c->Render();
    }
}
