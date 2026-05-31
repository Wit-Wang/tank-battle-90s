#pragma once

#ifdef TANKGAME_SERVER
#include "utils/raylib_stubs.h"
#else
#include "raylib.h"
#endif
#include <vector>

/// 粒子
struct Particle {
    Vector2 position;
    Vector2 velocity;
    float   lifetime;
    float   maxLifetime;
    float   size;
    Color   color;
};

/// 扩散环效果
struct RingEffect {
    Vector2 position;
    float   radius;
    float   maxRadius;
    float   lifetime;
    float   maxLifetime;
    float   thickness;
    Color   color;
};

/// 粒子系统：爆炸、击中等视觉效果
class ParticleSystem {
public:
    void Emit(Vector2 position, int count, Color color, float speed = 100.f);
    void EmitExplosion(Vector2 position, Color teamColor);
    void EmitRing(Vector2 position, float maxRadius, Color color, float duration = 0.5f);
    void Update(float dt);
    void Render();

private:
    std::vector<Particle> particles_;
    std::vector<RingEffect> rings_;
};
