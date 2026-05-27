# Breakout Game

基于 [Raylib](https://www.raylib.com/) 与 C++17 的双人打砖块游戏。支持 JSON 数据驱动关卡、存档读档、性能优化（对象池 + 空间网格碰撞）、运行时关卡编辑，以及暗色科技风界面与通关庆祝特效。

仓库：[LLNE-Collab/my-first-repo](https://github.com/LLNE-Collab/my-first-repo)

---

## 项目简介

玩家用两套键位控制挡板，击碎砖块、收集道具、通关多关卡。核心设计将**逻辑与数据分离**：

- **配置**：`config.json`（屏幕、球、挡板、性能参数）
- **关卡**：`levels/levelN.json`（砖块布局，无需重新编译）
- **存档**：`save.json`（关卡、分数、生命、道具剩余时间）

课程相关实现涵盖：第十周性能优化（`GetTime` / `TraceLog`、粒子对象池、网格碰撞）；第十一周数据持久化（JSON 关卡、存档、继续游戏、编辑模式）。

---

## 工程结构

```
breakout_project/
├── CMakeLists.txt      # 构建配置（Raylib + nlohmann/json）
├── config.json         # 全局游戏参数
├── include/
│   ├── Game.h          # 游戏实体与 Game 主类声明
│   └── JsonIO.h        # JSON 读写与容错
├── src/
│   ├── main.cpp        # 程序入口
│   ├── Game.cpp        # 游戏主逻辑
│   └── JsonIO.cpp
├── levels/
│   ├── level1.json ~ level5.json   # 五关不同难度布局
├── tests/
│   └── test_logic.cpp  # 碰撞逻辑单元测试
├── docs/               # 课程补充文档（可选阅读）
└── build/              # 推荐 out-of-source 构建目录（不提交 Git）
```

运行时生成（已加入 `.gitignore`）：`save.json`、`leaderboard.txt`。

---

## 依赖

| 依赖 | 用途 |
|------|------|
| CMake ≥ 3.14 | 构建 |
| C++17 编译器 | g++ / clang++ |
| Raylib | 图形、输入 |
| nlohmann/json | 配置与关卡、存档（CMake 自动 Fetch） |

Linux 可先安装系统 Raylib（可选，否则 CMake 自动下载）：

```bash
sudo apt install libraylib-dev cmake build-essential
```

---

## 编译与运行

```bash
cd breakout_project
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

构建完成后会将 `config.json` 与 `levels/` 复制到 `build/`。**请在 `build` 目录下运行**，以便正确加载相对路径资源：

```bash
./breakout
```

### 运行单元测试

```bash
cd build
ctest --output-on-failure
```

---

## 视觉效果

### 暗色科技风背景

主菜单、选关页、**游戏中**、暂停与通关庆祝界面统一使用深蓝黑网格背景（`#0A0C14` 色系），砖块带青色描边，HUD 文字为浅色以便阅读。

### 主菜单 / 选关页动态背景

| 界面 | 效果 |
|------|------|
| **主菜单** | 后台模拟实机：球、双挡板、砖块碰撞、道具下落；半透明遮罩上的菜单按钮 |
| **选关页** | 同上，按 Level 1–5 **随机顺序**轮播，每关约 **5 秒** 后切换下一关布局 |

### 游戏中

- 与菜单相同的科技风底色与砖块描边
- 关卡 JSON 中的背景图以约 **10%** 透明度叠加（若资源存在）

---

## 游戏流程

### 主菜单

| 按钮 | 说明 |
|------|------|
| **CONTINUE** | 仅玩过 **Select Game** 战役且存在进度时可用；**直接进入未完成的关卡**（不经过选关页） |
| **RANDOM GAME** | 随机生成砖块，本局**不写入存档** |
| **SELECT GAME** | 进入选关页，选择 1–5 关开始战役 |
| **SETTINGS** | 排行榜 |
| **QUIT** | 退出 |

### 选关页（仅 SELECT GAME）

- 5 个关卡卡片：上方为布局预览，下方为 **Level N** 标签
- 鼠标悬停：手型光标，卡片略微放大
- 点击某一关开始战役
- 背景持续播放各关“实机模拟”动画（随机顺序，每关 5 秒）
- **ESC** 返回主菜单

### 战役存档（CONTINUE）

- 须先通过 **Select Game** 开始过一关，才会生成 `save.json` 且带 `"campaign": true`
- **Continue** 直接加载存档中的关卡、分数、生命并开局
- 未玩过战役时 **Continue** 灰色不可点
- 暂停 **Q** 退出会保存战役进度；**Random Game** 不保存

### 通关庆祝（清关后）

#### Select Game（战役模式）

1. 清关后进入 **LEVEL_CLEAR** 状态，画面定格在通关瞬间
2. **2.5 秒** 烟花庆祝动画
3. 弹出对话框：
   - **Next Level**：进入下一关（进度已写入存档）
   - **Quit**：返回主菜单
4. 通关 **第 5 关**（战役完结）：仅 **Quit to Results**，进入 Game Over 与排行榜

#### Random Game

1. 同样 **2.5 秒** 烟花庆祝
2. 结束后**自动**生成并加载下一局随机关卡（无对话框）

### Random Game 字母障碍（难度机制）

仅 **RANDOM GAME** 生效；**Select Game / Continue** 不会出现障碍。

| 规则 | 说明 |
|------|------|
| **解锁** | 点击 **RANDOM GAME** 进入首局即开始刷新障碍（无需先通关多局） |
| **外观** | 随机 **A–Z 大写字母**（5×7 点阵），颜色为橙 / 品红 / 淡紫 / 珊瑚等（**非**红绿蓝黄），在暗色背景下易辨认 |
| **出现位置** | 上、下、左、右四边随机选一边生成，向对侧移动 |
| **离场** | 从上进入则落出下边界消失；从下进入则移出上边界；左→右、右→左同理 |
| **挡板** | 碰到障碍：生命 **-1**，障碍消失，挡板回到本关初始位置 |
| **球** | 碰到障碍：**反弹**（按碰撞面反射） |
| **难度曲线** | 通关次数越多，刷新概率越高（约 14% 起，上限约 72%），移动速度略增 |
| **HUD** | Random 模式下右上角显示 `Hazards ON (xx%)` |

### 游戏中操作

| 键位 | 玩家 1（WASD） | 玩家 2（方向键） |
|------|----------------|------------------|
| 移动挡板 | W A S D | ↑ ← ↓ → |

| 按键 | 功能 |
|------|------|
| **Esc** | 暂停 |
| **C** | 暂停后继续 |
| **Q** | 暂停并返回主菜单（战役自动存档） |
| **F5** | 手动存档（仅战役） |

### 性能调试（第十周）

| 按键 | 功能 |
|------|------|
| **G** | 显示/隐藏碰撞网格 |
| **B** | 网格碰撞 ↔ 朴素 O(N) 切换 |
| **Space** | 在球位置生成 100 个粒子（压测对象池） |

### 关卡编辑（第十一周加分）

| 按键 | 功能 |
|------|------|
| **E** | 进入/退出编辑模式 |
| **鼠标左键** | 添加砖块 |
| **鼠标右键** | 删除砖块 |
| **S** | 保存布局到 `levels/custom.json` |

---

## 功能列表

- [x] 双人挡板、球物理、砖块碰撞与道具
- [x] 5 关 JSON 驱动布局（选关页预览 + `pattern` / `layout` 两种格式）
- [x] **RANDOM GAME** 随机砖块（不存档），清关后烟花并自动下一局
- [x] **RANDOM GAME** 首局即出现字母障碍（四向移动、挡板扣命、球反弹、通关后难度递增）
- [x] **SELECT GAME** 选关界面（悬停放大 + 手型光标 + 动态背景轮播）
- [x] 战役清关：烟花 → **Next Level** / **Quit** 对话框
- [x] 主菜单 / 选关 / 游戏中统一暗色科技风背景
- [x] 存档 / 读档（`save.json`，版本号 v2，支持 v1 迁移）
- [x] 主菜单 **Continue** 直接续关
- [x] JSON 缺失或解析失败时回退默认配置（`LoadJSONWithFallback`）
- [x] 粒子对象池（无运行时 `new/delete`）
- [x] 8×6 空间网格碰撞 + 可视化调试
- [x] `GetTime` / `GetFrameTime` / `TraceLog` 性能测量与 HUD
- [x] 运行时关卡编辑并导出 JSON
- [x] 排行榜（`leaderboard.txt`）

---

## 关卡 JSON 简要说明

**pattern 格式**（`level1.json`）：每行字符串，`R/G/B/Y` 为砖块，`.` 为空。

**layout 格式**（`level2.json`）：`bricks.layout` 二维整数数组，`0` 为空，`color_map` 映射颜色。

详见 `docs/DATA_PERSISTENCE.md`。

---

## 补充文档

| 文件 | 内容 |
|------|------|
| [docs/DATA_PERSISTENCE.md](docs/DATA_PERSISTENCE.md) | 存档格式、读档流程、编辑模式 |
| [docs/PERFORMANCE.md](docs/PERFORMANCE.md) | 性能优化与对比实验 |
| [docs/PERF_REPORT.md](docs/PERF_REPORT.md) | 性能测量实现说明 |

---

## 许可证

课程作业项目；第三方库遵循各自许可证（Raylib、nlohmann/json、GoogleTest）。
