#pragma once

#include <functional>
#include <vector>
#include <unordered_map>
#include <cstdint>

/// 事件类型枚举
enum class EventType : uint8_t {
    TankDestroyed,
    BulletFired,
    WallDestroyed,
    BaseDestroyed,
    PowerUpCollected,
    ScoreChanged,
    PlayerDied,
    GameOver,
};

/// 事件数据
struct Event {
    EventType type;
    int       sourceId = -1;   // 触发事件的实体 ID
    int       value    = 0;    // 通用数值 (分数、伤害等)
    float     x       = 0.f;   // 事件发生位置
    float     y       = 0.f;
    int       team    = -1;    // 相关队伍
};

/// 事件监听器接口 (观察者模式)
class IEventListener {
public:
    virtual ~IEventListener() = default;
    virtual void OnEvent(const Event& event) = 0;
};

/// 事件分发器 (单例)：解耦游戏各系统间的通信
class EventSystem {
public:
    static EventSystem& Instance();

    void Subscribe(EventType type, IEventListener* listener);
    void Unsubscribe(EventType type, IEventListener* listener);
    void Dispatch(const Event& event);
    void Clear();

    EventSystem(const EventSystem&) = delete;
    EventSystem& operator=(const EventSystem&) = delete;

private:
    EventSystem() = default;

    std::unordered_map<EventType, std::vector<IEventListener*>> listeners_;
};
