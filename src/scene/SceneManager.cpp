#include "SceneManager.h"
#include "MenuScene.h"
#include "raylib.h"

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
    if (!scenes_.empty()) {
        scenes_.back()->Exit();
        scenes_.pop_back();
    }
    scenes_.push_back(std::move(scene));
    scenes_.back()->Enter();
}

void SceneManager::StartGame(std::unique_ptr<Scene> scene) {
    while (scenes_.size() > 2) {
        scenes_.back()->Exit();
        scenes_.pop_back();
    }
    scenes_.push_back(std::move(scene));
    scenes_.back()->Enter();
}

void SceneManager::ReturnTo(std::unique_ptr<Scene> fallback,
                             std::function<bool(const Scene&)> predicate) {
    for (int i = static_cast<int>(scenes_.size()) - 1; i >= 0; i--) {
        if (predicate(*scenes_[i])) {
            while (static_cast<int>(scenes_.size()) > i + 1) {
                scenes_.back()->Exit();
                scenes_.pop_back();
            }
            scenes_.back()->Exit();
            scenes_.pop_back();
            scenes_.push_back(std::move(fallback));
            scenes_.back()->Enter();
            return;
        }
    }
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
//  延迟导航 (在 Update 回调内安全使用)
// ============================================================

void SceneManager::PostReturnToMenu() {
    pendingAction_ = PendingAction::ReturnToMenu;
    pendingScene_.reset();
    pendingCustomAction_ = nullptr;
}

void SceneManager::PostPopScene() {
    pendingAction_ = PendingAction::PopScene;
    pendingScene_.reset();
    pendingCustomAction_ = nullptr;
}

void SceneManager::PostSetupNext(std::unique_ptr<Scene> scene) {
    pendingAction_ = PendingAction::SetupNext;
    pendingScene_ = std::move(scene);
    pendingCustomAction_ = nullptr;
}

void SceneManager::PostPushScene(std::unique_ptr<Scene> scene) {
    pendingAction_ = PendingAction::PushScene;
    pendingScene_ = std::move(scene);
    pendingCustomAction_ = nullptr;
}

void SceneManager::PostAction(std::function<void()> action) {
    pendingAction_ = PendingAction::Custom;
    pendingScene_.reset();
    pendingCustomAction_ = std::move(action);
}

void SceneManager::ProcessPendingActions() {
    switch (pendingAction_) {
        case PendingAction::ReturnToMenu:
            TraceLog(LOG_INFO, "SceneManager: executing PostReturnToMenu");
            ReturnToMenu();
            break;
        case PendingAction::PopScene:
            TraceLog(LOG_INFO, "SceneManager: executing PostPopScene");
            PopScene();
            break;
        case PendingAction::SetupNext:
            TraceLog(LOG_INFO, "SceneManager: executing PostSetupNext");
            SetupNext(std::move(pendingScene_));
            break;
        case PendingAction::PushScene:
            TraceLog(LOG_INFO, "SceneManager: executing PostPushScene");
            PushScene(std::move(pendingScene_));
            break;
        case PendingAction::Custom:
            TraceLog(LOG_INFO, "SceneManager: executing PostAction (custom)");
            if (pendingCustomAction_) {
                pendingCustomAction_();
                pendingCustomAction_ = nullptr;
            }
            TraceLog(LOG_INFO, "SceneManager: PostAction (custom) done");
            break;
        case PendingAction::None:
            break;
    }
    pendingAction_ = PendingAction::None;
    pendingScene_.reset();
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

    // 延迟导航: 场景在 Update 内调用 Post* 方法后, 在此执行实际切换
    ProcessPendingActions();
}

void SceneManager::Render() {
    if (scenes_.empty()) return;

    // 覆盖层：先渲染下层
    if (scenes_.back()->IsOverlay() && scenes_.size() >= 2) {
        scenes_[scenes_.size() - 2]->Render();
    }

    scenes_.back()->Render();
}
