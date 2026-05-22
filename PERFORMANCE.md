# 性能优化报告（Breakout）

## 1. 测量方法

- **内置计时器**：`GetTime()` 分段测量球–砖碰撞、粒子更新、整帧 `UpdatePlaying`。
- **控制台日志**：每 60 帧 `TraceLog` 输出各模块耗时与碰撞检测次数。
- **游戏内 HUD**：左上角 FPS、Physics/Particles 耗时条、碰撞检测次数、估算 DrawCall。

运行游戏后进入 PLAY，观察 HUD；终端可见详细日志。

## 2. 瓶颈分析

| 瓶颈 | 优化前 | 优化后 |
|------|--------|--------|
| 球–砖碰撞 O(N) | 每帧遍历全部活跃砖块 | 8×6 网格，只查球所在格及相邻 8 格 |
| 粒子 new/delete | 未使用（已用对象池） | 固定数组 `MAX_PARTICLES=1000`，无堆分配 |
| DrawCall | 每砖块一次绘制 | 概念层面统计（`DrawCalls~`），批绘制为选修 |

## 3. 已实现优化

### A. 空间划分（网格法）— 快组要求

- 屏幕划分为 `grid_cols × grid_rows`（默认 8×6）。
- 关卡加载时 `RebuildCollisionGrid()` 将砖块索引放入对应网格。
- 碰撞时只检测球所在格及周围 3×3 邻域。
- **G**：显示/隐藏网格线与球所在格高亮（教学演示）。
- **B**：在网格法与朴素 O(N) 之间切换，对比 `Checks` 数值。

### B. 对象池（粒子）— 慢组要求

- `particlePool_[MAX_PARTICLES]` + `particleActive_[]`。
- `EmitParticlesAtBrick` / `UpdateParticles` 复用槽位，无 `new`/`delete`。
- **Space**：在球位置一次生成 100 个粒子，观察帧率与 Part Δt。

### C. 性能预算（参考 60 FPS ≈ 16.6 ms/帧）

- UI 中 Physics / Particles 条以 4 ms 为满格，便于目视是否超支。

## 4. 对比实验步骤（提交用）

1. 编译并运行：`cd build && ./breakout`（需在含 `config.json` 的目录运行）。
2. 开始游戏，进入第 2 关（砖块较多）。
3. 按 **B** 切换到 **Naive**，记录 HUD 中 `Checks` 与 `Physics` ms。
4. 再按 **B** 切回 **Grid**，记录同样数据。
5. 按 **Space** 多次，对比粒子压力下的 `Part Δt` 与 FPS。
6. 按 **G** 截图网格演示界面。

**预期**：砖块多时 Grid 的 `Checks` 明显小于 `activeBrickCount_`；Naive 的 `Checks` 接近活跃砖块数。

## 5. 可选进阶

- Linux：`perf stat ./breakout`
- [Tracy](https://github.com/wolfpld/tracy) 接入 C++ 做帧级分析
- 调整 `config.json` 中 `grid_cols` / `grid_rows`（过小/过大都可能变慢，需实验）

## 6. 结论

优化遵循 **测量 → 分析 → 验证**：先暴露耗时与检测次数，再用网格与对象池降低无效工作。打砖块场景砖块静止、球移动，网格预计算 + 邻域查询是性价比最高的碰撞优化。
