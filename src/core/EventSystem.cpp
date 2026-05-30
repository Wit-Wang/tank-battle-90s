#include "EventSystem.h"

EventSystem& EventSystem::Instance() {
    static EventSystem instance;
    return instance;
}

void EventSystem::Subscribe(EventType type, IEventListener* listener) {
    listeners_[type].push_back(listener);
}

void EventSystem::Unsubscribe(EventType type, IEventListener* listener) {
    auto& vec = listeners_[type];
    std::erase(vec, listener);
}

void EventSystem::Dispatch(const Event& event) {
    auto it = listeners_.find(event.type);
    if (it != listeners_.end()) {
        for (auto* listener : it->second) {
            listener->OnEvent(event);
        }
    }
}

void EventSystem::Clear() {
    listeners_.clear();
}
