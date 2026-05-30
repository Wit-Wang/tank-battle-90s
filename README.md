# Tank Battle 90s

90 年代街机风格俯视角坦克 2v2 对战游戏。

## 特性

- **三种模式** — 传统对战 / 攻防战 / 各自为战
- **热座模式** — 本地多人，人/AI 混合，始终 2v2
- **联机模式** — 局域网 Host-Authoritative 对战
- **4 种坦克** — LIGHT / MEDIUM / HEAVY / SPEED，数据驱动属性表
- **程序化资源** — 纹理和音效全部代码生成，零外部依赖
- **语义化场景栈** — 6 种导航语义，栈自动维护

## 技术栈

C++23 / CMake / raylib 5.5 / Winsock2 (LAN)

## 构建

```bash
cmake -B build
cmake --build build
```

运行：`./build/Debug/TankGame.exe`

要求：CMake 3.20+，C++23 编译器。raylib 通过 FetchContent 自动获取。

## 项目结构

```
tankGame/
├── CMakeLists.txt
├── assets/
│   ├── maps/            # 地图文件 (.txt)，按模式分目录
│   ├── textures/        # 精灵图
│   └── fonts/           # 字体
├── docs/
│   ├── architecture.md  # 架构设计
│   ├── game-flow.md     # 场景流转
│   └── oo-design.md     # 设计模式
└── src/
    ├── core/            # 引擎层
    ├── ecs/             # Entity-Component 系统
    ├── game/            # 游戏逻辑
    ├── net/             # 网络层
    ├── scene/           # 场景层
    ├── ui/              # HUD
    └── utils/           # 工具库
```

详见 [docs/](docs/) 下的设计文档。
