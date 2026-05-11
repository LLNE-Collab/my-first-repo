# 性能优化与测量说明（作业提交用）

本文说明**如何测量**、**测到了什么**、**为什么选对象池与碰撞粗筛**，并给出**优化前后对比表模板**（请在本地实测后把「你的数据」列填完整，并附 1～2 张截图：游戏内 UI 左上角 + 终端 `Total: …ms | Physics: …` 日志）。

---

## 1. 测量工具与代码位置（可复查）

| 项目 | 实现位置 |
|------|----------|
| 整帧 `UpdatePlaying` 墙钟时间 | `Game::UpdatePlaying()` 开头 `startFrame = GetTime()` → 各出口及末尾 `RecordUpdatePlayingLatency(startFrame)` |
| 总耗时存盘 + 日志 | `Game::RecordUpdatePlayingLatency()`：每 **60** 帧 `TraceLog(..., "Total: %.2fms \| Physics: %.2fms \| Particles: %.2fms (Active: %d)", ...)` |
| 粒子子段 | `startParticle` → `UpdateParticles(dt)` → `lastParticleUpdateMs_`（`Game::UpdateParticles` 实现池内更新） |
| Physics 子段 | `startCollision` → 球–砖碰撞循环 → `lastBrickCollisionMs_`（含 AABB 粗筛 + `CheckCollisionCircleRec`） |
| 帧时间 | `GetFrameTime()` 存为 `dt` 传入 `UpdateParticles(dt)` 等；`DrawFPS()`（UI） |
| 实时 overlay | `Game::DrawUI()`：数值 + **绿/蓝性能条**（满条表示 4 ms，绿=Physics，蓝=Particles） |

**操作提示**：进入游戏 **PLAY** 后才会执行 `UpdatePlaying`；在菜单停留时通常看不到上述 `Total:` 日志。建议进关卡连续碎砖，观察 **Particles** 数值/蓝条与 **Physics** 绿条的相对比例。粒子更新写在球–砖碰撞**之前**，是为了在球重生等待期内粒子仍能每帧衰减（与讲义里「先写碰撞再写粒子」的伪代码顺序不同，属有意取舍）。

---

## 2. 找瓶颈：因果链（老师要的逻辑）

1. **代码层面（优化前）**  
   - 粒子使用 `std::vector<Particle>`：`push_back` 可能触发重分配；用 `erase(remove_if)` 删除死亡粒子为 **O(N)** 移动。大量爆炸时主线程容易出现尖峰。  
   - 球尾迹 `std::vector<TrailPoint>`：每帧 `push_back` + 满时 `erase(begin)`，同样有堆分配与移动风险。  

2. **运行层面（优化后）**  
   - 在 **PLAYING** 状态下看 UI / 日志：通常 **Particles（蓝条）** 会随活跃粒子数上升而变长；**Physics（绿条）** 在 **AABB 粗筛** 下多数帧接近 **0.00x ms**。  
   - 若你本地出现相反（砖块耗时 dominant），说明关卡砖块极多或球速导致碰撞遍历更热，可再考虑空间划分；当前数据驱动结论以你机器 overlay 为准。

3. **结论句式（可直接写进报告）**  
   「用 `GetTime()` 对 `UpdatePlaying` 及粒子/砖块两段分别计时；对比发现 **粒子更新在粒子密集时占比更高**，与 `vector` + `erase` 的复杂度分析一致，故采用 **固定数组对象池**；球尾迹改为 **定长环形缓冲** 消除同类问题；砖块碰撞增加 **轴对齐包围盒粗筛** 降低 `CheckCollisionCircleRec` 调用次数。」

---

## 3. 已实施的优化（与代码对应）

| 优化 | 说明 |
|------|------|
| **B：粒子对象池** | `particlePool_[MAX_PARTICLES]` + `particleActive_[]`，`EmitParticlesAtBrick` 只找空槽；死亡只清标记；`activeParticleCount_` 维护活跃数 |
| **碰撞粗筛** | 球心 ± 半径与砖块矩形不相交则 **continue**，再调用 `CheckCollisionCircleRec` |
| **Ball 尾迹** | `MAX_TRAIL=10` 环形缓冲，无 `vector` 堆分配 |

---

## 4. 优化前后数据对比（请本地填数 + 截图）

> **优化前**：若你仍保留旧提交，可 `git checkout <旧commit>` 编译运行，同场景记录 FPS 与（若有）总耗时。  
> **优化后**：当前 `main` 编译，同场景记录。

| 场景（请自填） | 优化前（vector 粒子） | 优化后（对象池 + 尾迹数组 + 砖块粗筛） |
|----------------|----------------------|----------------------------------------|
| 典型游玩（粒子较少） | 你的 FPS：____ ；Update：____ ms | 你的 FPS：____ ；Update：____ ms |
| 连续碎砖（粒子多，Active 接近上限） | 你的 FPS：____ ；`Particles` 段：____ ms | 你的 FPS：____ ；`Particles` 段：____ ms |

**本仓库在 CI/WSL 一次短跑中曾见类似日志（仅供参考，非对比实验）**：  
`Total: 0.02ms | Physics: 0.00ms | Particles: 0.00ms (Active: 0)`（菜单或未进入 PLAY 时子段为 0 属正常）。

---

## 5. 截图清单（满足作业「截图或数据」）

1. 游戏画面：可见 `DrawFPS`、`Update`、活跃粒子数、`Part Δt` / `Physics` 文本及绿/蓝性能条。  
2. 终端：`Total: ...ms | Physics: ...ms | Particles: ...ms (Active: n)` 若干行。

---

## 6. 复现命令

```bash
cmake -S . -B build && cmake --build build
./build/breakout
# 点击 PLAY，进入关卡后打砖块，观察 UI 与终端
```

工作目录建议在项目根目录，以便加载 `config.json` / `assets`（若使用）。
