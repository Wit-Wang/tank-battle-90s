# src/ — 源代码

Tank Battle 90s 的 C++23 源代码，四层架构。

## 目录

```
src/
├── main.cpp
├── core/           # 引擎层：窗口、输入、音频、资源、事件
├── ecs/            # Entity-Component 系统
├── game/
│   ├── tank/       #   Tank、TankFactory、IController
│   ├── entities/   #   Bullet、Wall、Base、PowerUp
│   ├── map/        #   GameMap、MapManager
│   ├── systems/    #   CollisionSystem、ParticleSystem
│   └── session/    #   GameSession、PlayerSlot
├── net/            # 网络层 (PIMPL 隔离 Winsock2)
├── scene/          # SceneManager + 14 Scenes
├── ui/             # HUD
└── utils/          # MathUtils、纹理/音效生成、随机数
```

## 依赖方向

```
Scene Layer → Game Logic Layer → ECS Layer → Core Engine → raylib
     ↓
Network Layer (PIMPL 隔离 Winsock2)
```

上层依赖下层，禁止反向依赖。
