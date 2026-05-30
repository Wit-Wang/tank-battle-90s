# ui/ — HUD

游戏内信息显示。

| 文件 | 职责 |
|---|---|
| `HUD.h/.cpp` | 模式相关 HUD：队伍标签、各槽位状态 (P1-AI3, 坦克类型, 存活/死亡) |

HUD 通过 `const GameSession&` 引用读取槽位状态，不修改游戏数据。标签根据游戏模式显示不同文字 (RED/BLUE、ATK/DEF 等)。
