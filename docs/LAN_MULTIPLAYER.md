# 局域网双人合作（第7课 网络编程）

## 架构

| 角色 | 职责 |
|------|------|
| **主机 HOST** | `listen` 等待连接；运行完整物理；广播 `state`（20Hz） |
| **客机 JOIN** | `connect` 到主机 IP；发送 `input`（挡板位置）；接收并渲染 `state` |

- 传输：**TCP**（可靠）
- 分帧：每条 JSON 以换行符 `\n` 结尾
- 默认端口：**5555**
- 玩家1（主机）：**WASD**
- 玩家2（客机）：**方向键**

## 消息类型

```json
{"type":"hello","role":"host"}
{"type":"start","level":1}
{"type":"input","x":450,"y":550}
{"type":"state","ball":{...},"p1":{...},"p2":{...},"score":0,"lives":3,"bricks":[true,false,...]}
{"type":"bye"}
```

## 操作步骤

### 同一台电脑测试

1. 终端 A：`cd build && ./breakout` → **LAN CO-OP** → **HOST**
2. 终端 B：`cd build && ./breakout` → **LAN CO-OP** → **JOIN** → IP 填 `127.0.0.1` → **CONNECT**

### 两台电脑（同一 WiFi / 局域网）

1. 主机：开房后记下界面显示的 **本机 IP**（如 `192.168.1.5`）
2. 客机：JOIN 输入主机 IP → CONNECT
3. Windows 防火墙需允许端口 **5555** 入站（主机）

## 源码位置

- `include/NetProtocol.h` — 协议与序列化
- `include/LanSession.h` / `src/LanSession.cpp` — socket 封装
- `Game.cpp` — 菜单、同步、主机权威逻辑

## 答辩要点

- **为什么 TCP**：存档/状态需要可靠到达；课程入门常用 TCP
- **为什么主机权威**：避免双方物理不同步；客机只发输入
- **非阻塞 socket**：游戏主循环每帧 `recv`/`accept`，不卡死窗口
