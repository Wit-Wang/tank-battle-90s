#pragma once

#include "Scene.h"
#include <vector>
#include <memory>
#include <functional>

/// 场景管理器：基于语义导航的栈式管理
///
/// 导航模型:
///   PushScene   — 压入覆盖层 (暂停等)，保留下层场景
///   PopScene    — 弹出当前层，恢复下层
///   SetupNext   — 设置流程中前进 (替换当前场景，栈不增长)
///   StartGame   — 清除设置链，压入 GameScene
///   ReturnTo    — 清除到栈中已有场景 (若不存在则清空后 Push)
///
/// 延迟导航 (Post* 方法):
///   在场景的 Update() 回调内，必须使用 Post* 方法代替立即导航方法，
///   以避免当前场景在自身 Update() 执行期间被销毁 (use-after-free)。
class SceneManager {
public:
    SceneManager() = default;

    // ---- 基础操作 ----

    /// 压入覆盖层 (暂停等)
    void PushScene(std::unique_ptr<Scene> scene);

    /// 弹出当前层，恢复下层场景
    void PopScene();

    // ---- 语义化导航 ----

    /// 设置流程前进：替换当前栈顶 (用于 Char→Team→Map)
    void SetupNext(std::unique_ptr<Scene> scene);

    /// 开始游戏：清除设置链，压入 GameScene
    void StartGame(std::unique_ptr<Scene> scene);

    /// 返回栈中指定类型的场景 (通过匹配谓词查找)
    /// 若找到，清除其上所有层；若未找到，清空后 Push
    void ReturnTo(std::unique_ptr<Scene> fallback,
                  std::function<bool(const Scene&)> predicate);

    /// 返回主菜单：搜索 MenuScene，清除其上所有层；未找到则清空后 Push
    void ReturnToMenu();

    // ---- 废弃接口 (兼容) ----
    void ResetToScene(std::unique_ptr<Scene> scene);

    // ---- 延迟导航 (在 Update 回调内安全使用) ----

    void PostReturnToMenu();
    void PostPopScene();
    void PostSetupNext(std::unique_ptr<Scene> scene);
    void PostPushScene(std::unique_ptr<Scene> scene);

    /// 延迟执行自定义操作 (可组合多个导航，用于需要捕获 move-only 对象的场景)
    void PostAction(std::function<void()> action);

    // ---- 查询 ----

    void Update(float dt);
    void Render();

    bool IsEmpty() const { return scenes_.empty(); }
    std::size_t Depth() const { return scenes_.size(); }

private:
    void ProcessPendingActions();

    std::vector<std::unique_ptr<Scene>> scenes_;

    // 延迟导航状态
    enum class PendingAction { None, ReturnToMenu, PopScene, SetupNext, PushScene, Custom };
    PendingAction pendingAction_ = PendingAction::None;
    std::unique_ptr<Scene> pendingScene_;
    std::function<void()> pendingCustomAction_;
};
