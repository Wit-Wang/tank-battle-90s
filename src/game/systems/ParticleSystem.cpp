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

void ParticleSystem::EmitExplosion(Vector2 position, Color teamColor) {
    // 核心火焰: 高速、明亮的小粒子
    for (int i = 0; i < 15; i++) {
        Particle p;
        p.position = position;
        float angle = Random::Float(0.f, 360.f) * DEG2RAD;
        float spd = Random::Float(120.f, 200.f);
        p.velocity = { std::cos(angle) * spd, std::sin(angle) * spd };
        p.lifetime = 0.f;
        p.maxLifetime = Random::Float(0.2f, 0.5f);
        p.size = Random::Float(2.f, 5.f);
        p.color = (Random::Float(0.f, 1.f) > 0.5f) ? YELLOW : ORANGE;
        particles_.push_back(p);
    }
    // 碎片: 中速、较大、队伍色
    for (int i = 0; i < 12; i++) {
        Particle p;
        p.position = position;
        float angle = Random::Float(0.f, 360.f) * DEG2RAD;
        float spd = Random::Float(60.f, 140.f);
        p.velocity = { std::cos(angle) * spd, std::sin(angle) * spd };
        p.lifetime = 0.f;
        p.maxLifetime = Random::Float(0.4f, 0.9f);
        p.size = Random::Float(3.f, 7.f);
        p.color = teamColor;
        particles_.push_back(p);
    }
    // 烟雾: 慢速、大、灰色
    for (int i = 0; i < 8; i++) {
        Particle p;
        p.position = position;
        float angle = Random::Float(0.f, 360.f) * DEG2RAD;
        float spd = Random::Float(20.f, 60.f);
        p.velocity = { std::cos(angle) * spd, std::sin(angle) * spd };
        p.lifetime = 0.f;
        p.maxLifetime = Random::Float(0.5f, 1.0f);
        p.size = Random::Float(5.f, 10.f);
        p.color = GRAY;
        particles_.push_back(p);
    }
    // 冲击波环
    EmitRing(position, 48.f, teamColor, 0.4f);
}

void ParticleSystem::EmitRing(Vector2 position, float maxRadius, Color color, float duration) {
    RingEffect r;
    r.position = position;
    r.radius = 4.f;
    r.maxRadius = maxRadius;
    r.lifetime = 0.f;
    r.maxLifetime = duration;
    r.thickness = 3.f;
    r.color = color;
    rings_.push_back(r);
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

    for (auto& r : rings_) {
        float t = r.lifetime / r.maxLifetime;
        r.radius = r.radius + (r.maxRadius - r.radius) * t;
        r.lifetime += dt;
    }

    std::erase_if(rings_, [](const RingEffect& r) {
        return r.lifetime >= r.maxLifetime;
    });
}

void ParticleSystem::Render() {
    for (const auto& p : particles_) {
        float alpha = 1.f - (p.lifetime / p.maxLifetime);
        Color c = ColorAlpha(p.color, alpha);
        DrawCircleV(p.position, p.size, c);
    }
    for (const auto& r : rings_) {
        float alpha = 1.f - (r.lifetime / r.maxLifetime);
        Color c = ColorAlpha(r.color, alpha);
        DrawRing(r.position, r.radius - r.thickness, r.radius, 0.f, 360.f, 32, c);
    }
}
