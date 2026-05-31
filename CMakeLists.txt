cmake_minimum_required(VERSION 3.20)
project(TankGame LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---- Shared game logic (used by both client and server) ----
set(SHARED_SOURCES
    src/core/AudioManager.cpp
    src/core/EventSystem.cpp
    src/core/InputManager.cpp
    src/core/ResourceManager.cpp
    src/ecs/Entity.cpp
    src/ecs/EntityManager.cpp
    src/ecs/components/ColliderComponent.cpp
    src/ecs/components/HealthComponent.cpp
    src/ecs/components/MovementComponent.cpp
    src/ecs/components/SpriteComponent.cpp
    src/game/entities/Base.cpp
    src/game/entities/Bullet.cpp
    src/game/entities/PowerUp.cpp
    src/game/entities/Wall.cpp
    src/game/map/GameMap.cpp
    src/game/map/MapManager.cpp
    src/game/session/GameSession.cpp
    src/game/systems/CollisionSystem.cpp
    src/game/systems/ParticleSystem.cpp
    src/game/tank/Tank.cpp
    src/game/tank/TankFactory.cpp
    src/game/tank/controller/AIController.cpp
    src/game/tank/controller/PlayerController.cpp
    src/net/NetController.cpp
    src/net/NetworkManager.cpp
    src/scene/GameNetworkHost.cpp
)

# ---- Server-only sources ----
set(SERVER_SOURCES
    src/server/main.cpp
    src/server/GameServer.cpp
)

# ---- Server (always built, no X11/OpenGL needed) ----
add_executable(TankGameServer ${SHARED_SOURCES} ${SERVER_SOURCES})
target_include_directories(TankGameServer PRIVATE src)
target_compile_definitions(TankGameServer PRIVATE TANKGAME_SERVER)
if(WIN32)
    target_link_libraries(TankGameServer PRIVATE ws2_32 winmm)
else()
    target_link_libraries(TankGameServer PRIVATE pthread m)
endif()

# ---- Client (Windows only — requires raylib/X11) ----
if(WIN32)
    include(FetchContent)
    FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG        5.5
        GIT_SHALLOW    TRUE
    )
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_GAMES    OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(raylib)

    set(CLIENT_SOURCES
        src/main.cpp
        src/core/Game.cpp
        src/core/Window.cpp
        src/scene/GameOverScene.cpp
        src/scene/GameScene.cpp
        src/scene/LANGameScene.cpp
        src/scene/LANLobbyScene.cpp
        src/scene/LANModeSelectScene.cpp
        src/scene/MapSelectScene.cpp
        src/scene/MenuScene.cpp
        src/scene/ModeSelectScene.cpp
        src/scene/PauseScene.cpp
        src/scene/SceneManager.cpp
        src/scene/ServerLobbyScene.cpp
        src/scene/SetupScene.cpp
        src/scene/TeamSelectScene.cpp
        src/ui/HUD.cpp
        src/utils/SoundGenerator.cpp
        src/utils/TextureGenerator.cpp
    )

    add_executable(${PROJECT_NAME} ${SHARED_SOURCES} ${CLIENT_SOURCES})
    target_include_directories(${PROJECT_NAME} PRIVATE src)
    target_link_libraries(${PROJECT_NAME} PRIVATE raylib ws2_32 winmm)

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
        COMMENT "Copying assets to client build directory"
    )
endif()
