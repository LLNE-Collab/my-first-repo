#pragma once

#include "raylib.h"
#include <vector>
#include <string>
#include <nlohmann/json.hpp>

// 使用 nlohmann/json 库处理配置
using json = nlohmann::json;

// 游戏状态枚举，替代散乱的 bool 标志
enum class GameState {
    MENU,
    NAME_INPUT,
    LEVEL_SELECT,
    SETTINGS,
    PLAYING,
    PAUSED,
    GAME_OVER,
    LEADERBOARD,
    WIN
};

// 砖块结构体
struct Brick {
    Rectangle rect;
    bool active;
    Color color;
};

// 游戏主类
class Game {
public:
    Game();
    ~Game();
    
    void Run(); // 主循环

private:
    // 核心流程
    void Update();
    void Draw();
    void ResetGame();
    
    // 状态更新函数
    void UpdateMenu();
    void UpdateNameInput();
    void UpdateLevelSelect();
    void UpdateSettings();
    void UpdatePlaying();
    void UpdatePaused();
    void UpdateGameOver();
    void UpdateLeaderboard();
    
    // 绘制辅助
    void DrawUI();
    void DrawHeart(Vector2 position, float size, Color color); // 绘制爱心
    
    // 数据持久化
    void LoadConfig();
    void SaveScore();
    void LoadLeaderboard();
    
    // 辅助工具
    Color JsonToColor(const json& j); // JSON 转颜色
    bool IsButtonClicked(Rectangle rec); // 按钮点击检测

    // 数据成员
    GameState state_;
    int screenWidth_, screenHeight_;
    
    // 游戏对象
    struct { Vector2 pos; Vector2 vel; float radius; Color color; } ball_;
    struct { Vector2 pos; float width, height; Color color; } paddle_;
    std::vector<Brick> bricks_;
    
    // 游戏数据
    std::string username_;
    int score_;
    int lives_;
    int level_;
    json config_;
    std::vector<std::pair<std::string, int>> leaderboardData_;
    
    // UI 缓存
    Rectangle btnPlay_, btnSettings_, btnQuit_, btnLeaderboard_, btnLevelSelect_;
};
