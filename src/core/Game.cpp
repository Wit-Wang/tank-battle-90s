#include "Game.h"
#include "scene/SceneManager.h"
#include "scene/MenuScene.h"
#include "utils/TextureGenerator.h"
#include "utils/SoundGenerator.h"
#include <stdexcept>
#include <cstdio>

Game::Game() = default;
Game::~Game() = default;

void Game::Init() {
    SetExitKey(KEY_NULL);  // 禁用 ESC 退出，由场景自行处理

    // 程序化生成所有资源 (不依赖外部文件)
    TextureGenerator::GenerateAll();
    SoundGenerator::GenerateAll();

    sceneManager_ = std::make_unique<SceneManager>();
    sceneManager_->PushScene(std::make_unique<MenuScene>(sceneManager_.get()));
}

void Game::Run() {
    Init();

    while (!window_.ShouldClose()) {
        float dt = GetFrameTime();

        try {
            sceneManager_->Update(dt);
        } catch (const std::exception& e) {
            TraceLog(LOG_ERROR, "EXCEPTION in Update: %s", e.what());
            // 清理场景栈并返回菜单
            sceneManager_.reset();
            sceneManager_ = std::make_unique<SceneManager>();
            sceneManager_->PushScene(std::make_unique<MenuScene>(sceneManager_.get()));
        } catch (...) {
            TraceLog(LOG_ERROR, "UNKNOWN EXCEPTION in Update");
            sceneManager_.reset();
            sceneManager_ = std::make_unique<SceneManager>();
            sceneManager_->PushScene(std::make_unique<MenuScene>(sceneManager_.get()));
        }

        window_.BeginFrame();
        sceneManager_->Render();
        window_.EndFrame();
    }

    Shutdown();
}

void Game::Shutdown() {
    sceneManager_.reset();
}
