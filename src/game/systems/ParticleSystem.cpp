#include "ParticleSystem.h"
#include "utils/Random.h"

void ParticleSystem::Emit(Vector2 position, int count, Color color, float speed) {
    for (int i = 0; i < count; i++) {
        Particle p;
        p.position = position;
        float angle = Random::Float(0.f, 360.f) * DEG2RAD;
        float spd = Random::Float(speed * 0.3f, speed);
        p.velocity = { std::cos(angle) * spd, std::sin(angle) * spd };
        p.lifetime = 0.f;
        p.maxLifetime = Random::Float(0.3f, 0.8f);
        p.size = Random::Float(2.f, 6.f);
        p.color = color;
        particles_.push_back(p);
    }
}

void ParticleSystem::Update(float dt) {
    for (auto& p : particles_) {
        p.position.x += p.velocity.x * dt;
        p.position.y += p.velocity.y * dt;
        p.velocity.x *= 0.95f;
        p.velocity.y *= 0.95f;
        p.lifetime += dt;
    }

    std::erase_if(particles_, [](const Particle& p) {
        return p.lifetime >= p.maxLifetime;
    });
}

void ParticleSystem::Render() {
    for (const auto& p : particles_) {
        float alpha = 1.f - (p.lifetime / p.maxLifetime);
        Color c = ColorAlpha(p.color, alpha);
        DrawCircleV(p.position, p.size, c);
    }
}
