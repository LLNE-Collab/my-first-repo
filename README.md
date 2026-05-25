Breakout Game
简介
一款基于 Raylib 的 2D 打砖块游戏，支持双人同屏、道具系统、JSON 多关卡与存档读档。
关卡布局与游戏参数由外部 JSON 配置，无需改代码重新编译；含性能优化（粒子对象池、空间网格碰撞）与运行时关卡编辑。

配置：config.json
关卡：levels/level1.json ~ level3.json
存档：save.json（自动生成）
仓库：LLNE-Collab/my-first-repo

编译运行
环境要求
CMake >= 3.14
C++17 编译器（g++ / clang++）
Raylib（可选系统安装；未安装时 CMake 会自动拉取）
nlohmann/json（CMake 自动 Fetch）
可选（Linux）：

sudo apt install cmake build-essential libraylib-dev
编译步骤
cd breakout_project
mkdir -p build && cd build
cmake ..
make -j$(nproc)
在 build 目录下运行（需能读到 config.json 与 levels/）：

./breakout
单元测试（可选）
cd build
ctest --output-on-failure
操作说明
主菜单
操作	说明
CONTINUE
有存档时继续游戏
NEW GAME / PLAY
新游戏
SETTINGS
排行榜
QUIT
退出
游戏中
玩家	移动挡板
玩家 1
W / A / S / D
玩家 2
方向键 ↑←↓→
按键	功能
Esc
暂停
C
暂停后继续
Q
返回主菜单（自动存档）
F5
手动存档
G
显示/隐藏碰撞网格（性能调试）
B
切换网格 / 朴素碰撞检测
Space
生成测试粒子
E
进入/退出关卡编辑模式
S
编辑模式下保存关卡到 levels/custom.json
说明：球开局自动运动，无需按空格发射；无单独 R 重开键，请用主菜单 NEW GAME。

功能列表
双人挡板、球物理、砖块碰撞
道具：挡板加宽、慢球等
3 关 JSON 驱动（pattern / layout 两种布局）
通关自动下一关
存档 / 读档 / 主菜单继续
排行榜（leaderboard.txt）
粒子对象池、网格碰撞优化
GetTime / TraceLog 性能测量
运行时关卡编辑并导出 JSON
breakout_project/
├── CMakeLists.txt
├── config.json
├── include/          # Game.h, JsonIO.h
├── src/              # Game.cpp, JsonIO.cpp, main.cpp
├── levels/           # level1.json ~ level3.json
├── tests/
├── docs/             # 数据持久化 / 性能说明
└── build/            # 编译输出（推荐在此运行）
