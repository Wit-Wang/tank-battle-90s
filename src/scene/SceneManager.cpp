#include "SceneManager.h"
#include "MenuScene.h"

// ============================================================
//  基础操作
// ============================================================

void SceneManager::PushScene(std::unique_ptr<Scene> scene) {
    if (!scenes_.empty()) {
        scenes_.back()->Exit();
    }
    scenes_.push_back(std::move(scene));
    scenes_.back()->Enter();
}

void SceneManager::PopScene() {
    if (scenes_.empty()) return;
    scenes_.back()->Exit();
    scenes_.pop_back();
    if (!scenes_.empty()) {
        scenes_.back()->Enter();
    }
}

// ============================================================
//  语义化导航
// ============================================================

void SceneManager::SetupNext(std::unique_ptr<Scene> scene) {
    // 替换栈顶，不增长栈深度
    // 用于设置流程: Lobby → CharSelect → TeamSelect → MapSelect
    if (!scenes_.empty()) {
        scenes_.back()->Exit();
        scenes_.pop_back();
    }
    scenes_.push_back(std::move(scene));
    scenes_.back()->Enter();
}

void SceneManager::StartGame(std::unique_ptr<Scene> scene) {
    // 清除设置链 (保留 Menu + Lobby)，压入 GameScene
    // 栈: [Menu, Lobby, CharSelect, TeamSelect, MapSelect]
    //   → [Menu, Lobby, GameScene]
    while (scenes_.size() > 2) {
        scenes_.back()->Exit();
        scenes_.pop_back();
    }
    scenes_.push_back(std::move(scene));
    scenes_.back()->Enter();
}

void SceneManager::ReturnTo(std::unique_ptr<Scene> fallback,
                             std::function<bool(const Scene&)> predicate) {
    // 从栈底向上搜索目标场景
    for (int i = static_cast<int>(scenes_.size()) - 1; i >= 0; i--) {
        if (predicate(*scenes_[i])) {
            // 找到目标：清除其上所有层
            while (static_cast<int>(scenes_.size()) > i + 1) {
                scenes_.back()->Exit();
                scenes_.pop_back();
            }
            // 当前栈顶就是目标，重新 Enter (刷新状态)
            scenes_.back()->Exit();
            scenes_.pop_back();
            scenes_.push_back(std::move(fallback));
            scenes_.back()->Enter();
            return;
        }
    }
    // 未找到：清空后 Push fallback
    if (!scenes_.empty()) {
        scenes_.back()->Exit();
    }
    scenes_.clear();
    scenes_.push_back(std::move(fallback));
    scenes_.back()->Enter();
}

// ============================================================
//  便捷方法
// ============================================================

void SceneManager::ReturnToMenu() {
    ReturnTo(
        std::make_unique<MenuScene>(this),
        [](const Scene& s) { return dynamic_cast<const MenuScene*>(&s) != nullptr; });
}

// ============================================================
//  废弃接口 (保持兼容)
// ============================================================

void SceneManager::ResetToScene(std::unique_ptr<Scene> scene) {
    if (!scenes_.empty()) {
        scenes_.back()->Exit();
    }
    scenes_.clear();
    scenes_.push_back(std::move(scene));
    scenes_.back()->Enter();
}

// ============================================================
//  主循环
// ============================================================

void SceneManager::Update(float dt) {
    if (scenes_.empty()) return;
    scenes_.back()->Update(dt);
}

void SceneManager::Render() {
    if (scenes_.empty()) return;

    // 覆盖层：先渲染下层
    if (scenes_.back()->IsOverlay() && scenes_.size() >= 2) {
        scenes_[scenes_.size() - 2]->Render();
    }

    scenes_.back()->Render();
}
