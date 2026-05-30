# core/ — 引擎层

对 raylib 的薄封装，提供游戏运行基础设施。

| 文件 | 职责 |
|---|---|
| `Types.h` | 游戏常量 (窗口 832x832、网格 64px)、GameMode、Direction 枚举 |
| `Game.h/.cpp` | 主循环：初始化 → Update/Render → 关闭 |
| `Window.h/.cpp` | raylib 窗口 RAII 封装 |
| `InputManager.h/.cpp` | 键盘输入查询 |
| `ResourceManager.h/.cpp` | 纹理/字体资源管理 (Meyers' Singleton) |
| `AudioManager.h/.cpp` | 音效播放管理 (Meyers' Singleton) |
| `EventSystem.h/.cpp` | 观察者模式事件分发 (Meyers' Singleton) |
| `Timer.h` | 轻量计时器 (冷却、无敌时间等) |
