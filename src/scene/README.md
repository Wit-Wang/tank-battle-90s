# scene/ — 场景层

基于状态模式的场景管理，支持三种模式 + 联机。

## SceneManager 导航语义

| 方法 | 语义 | 使用场景 |
|---|---|---|
| `PushScene` | 压入覆盖层 | Menu→ModeSelect, Game+Pause |
| `PopScene` | 弹出覆盖层 | Pause→Game |
| `SetupNext` | 流程前进 (替换栈顶) | Setup→MapSelect, Game→GameOver |
| `StartGame` | 清除设置链，压入 Game | MapSelect→Game |
| `ReturnToMenu` | 回主菜单 | 任何场景 ESC |

## 热座模式 (7 场景)

| 场景 | 职责 |
|---|---|
| `MenuScene` | 标题画面 |
| `ModeSelectScene` | 模式选择 (传统/攻防/各自为战) |
| `SetupScene` | 人/AI + 坦克 + 队伍配置 |
| `MapSelectScene` | 按模式过滤地图选择 (平滑动画) |
| `GameScene` | 对战主循环 (含重生 + 网络 Host 广播) |
| `PauseScene` | 暂停覆盖层 (IsOverlay) |
| `GameOverScene` | 模式相关结算 |

## 联机模式 (4 场景/类)

| 场景 | 职责 |
|---|---|
| `LANModeSelectScene` | Host / Join 选择 |
| `LANLobbyScene` | Host: 选图/队伍/坦克 (Q/E); Client: 选坦克 + IP 输入 |
| `LANGameScene` | 客户端：接收状态渲染 + 发送输入 |
| `GameNetworkHost` | 辅助类：网络广播/同步 |

## NetworkManager 所有权

```
LANLobbyScene (unique_ptr)
  ↓ ReleaseNetwork()
GameScene / LANGameScene (NetworkManagerPtr, PIMPL 删除器)
  ↓ 析构自动清理
```
