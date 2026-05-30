#include "HealthComponent.h"

void HealthComponent::TakeDamage(int amount) {
    hp_ -= amount;
    if (hp_ <= 0) {
        hp_ = 0;
        if (onDeath) onDeath();
    }
}

void HealthComponent::Heal(int amount) {
    hp_ = std::min(hp_ + amount, maxHp_);
}
