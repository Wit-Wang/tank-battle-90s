#pragma once

/// 轻量计时器：用于冷却、无敌时间等
class Timer {
public:
    explicit Timer(float duration = 0.f) : duration_(duration), elapsed_(0.f) {}

    void Reset() { elapsed_ = 0.f; }
    void Update(float dt) { elapsed_ += dt; }

    bool IsFinished() const { return elapsed_ >= duration_; }
    float GetProgress() const { return (duration_ > 0.f) ? elapsed_ / duration_ : 1.f; }
    float Remaining() const { return duration_ - elapsed_; }

    void SetDuration(float d) { duration_ = d; }

private:
    float duration_;
    float elapsed_;
};
