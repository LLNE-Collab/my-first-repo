# 第十一周：数据持久化与关卡编辑器

## 已实现功能对照

| 作业要求 | 实现 |
|---------|------|
| 砖块布局从 JSON 读取 | `levels/levelN.json`，支持 `pattern` 字符串与 `bricks.layout` 二维数组 |
| 存档 / 读档 | `save.json`，版本号 `version: 2` |
| 启动检测存档并继续 | 主菜单 **CONTINUE** 按钮 |
| ≥3 关，通关自动下一关 | `CountLevelFiles()` + 清关后 `StartLevelLoad(next)` |
| JSON 错误处理 | `LoadJSONWithFallback` + `TraceLog` 警告 |
| 编辑模式（加分） | **E** 进入，左键添加 / 右键删除 / **S** 保存到 `levels/custom.json` |
| 存档版本迁移 | v1 读档后自动升级并写回 v2 |

## 关卡 JSON 格式

### 方式 A：`pattern`（第 1、3 关）

```json
"pattern": ["RRRRRRRR", "GG....GG"]
```

- `.` / `0` / 空格 = 空
- `R/G/B/Y/C/O/P` = 颜色

### 方式 B：`bricks.layout`（第 2 关，与 PPT 一致）

```json
"bricks": {
  "rows": 5, "cols": 10,
  "width": 70, "height": 18, "spacing": 4,
  "offset_x": 50, "offset_y": 70,
  "layout": [[1,1,0,...], ...],
  "color_map": { "1": "red", "2": "yellow" }
}
```

## 存档格式 `save.json`

```json
{
  "version": 2,
  "current_level": 2,
  "score": 120,
  "lives": 3,
  "powerups": {
    "paddle_extend_remaining": 2.5,
    "slow_ball_remaining": 0
  }
}
```

## 操作说明

| 按键 | 作用 |
|------|------|
| 主菜单 CONTINUE | 从存档继续 |
| 主菜单 NEW GAME / PLAY | 新游戏（删除旧存档） |
| F5 | 游戏中手动存档 |
| 暂停后 Q | 返回菜单并自动存档 |
| E | 切换编辑模式 |
| S | 编辑模式下保存布局 |

## 演示视频建议流程

1. NEW GAME → 进入第 1 关 → 打掉部分砖块 → F5 存档  
2. 退出到菜单（暂停 Q）→ **CONTINUE** → 仍在同一关卡，分数/生命保留  
3. 通关第 1 关 → 自动进入第 2 关（JSON layout）  
4. 按 E 进入编辑模式 → 放置砖块 → S 保存 custom.json  

## 运行

```bash
cd build && cmake .. && make
./breakout
```

构建后会自动复制 `config.json`、`levels/`、`assets/` 到 `build/` 目录。
