#pragma once

#include <raylib.h>
#include <nlohmann/json.hpp>

#include <future>
#include <atomic>
#include <mutex>
#include <thread>
#include <vector>
#include <string>
#include <memory>
#include <utility>

using json = nlohmann::json;

// =====================
// 游戏状态
// =====================
enum class GameState {
    MENU,
    PLAYING,
    PAUSED,
    GAME_OVER,
    LEADERBOARD
};

// =====================
// 道具类型
// =====================
enum class PowerUpType {
    PADDLE_EXTEND = 0,
    MULTI_BALL    = 1,
    SLOW_BALL     = 2
};

// =====================
// 砖块类型
// =====================
enum class BrickType {
    Normal = 1,
    Gold   = 2,
    Bomb   = 3
};

enum class LoadState {
    IDLE,
    LOADING,
    DONE
};

class Game;

// =====================
// 得分计算器
// =====================
class ScoreCalculator {
public:
    [[nodiscard]] int CalculateBaseScore(BrickType brickType) const noexcept;
    [[nodiscard]] int CalculateScore(BrickType brickType, int combo = 0) const noexcept;

private:
    static constexpr int comboBonus_ = 2;
};

// =====================
// Ball
// =====================
class Ball {
public:
    Ball(Vector2 pos = {0, 0}, Vector2 vel = {0, 0}, float r = 0.0f, Color c = BLACK);
    ~Ball();

    void Update(float dt);
    void Draw() const;

    Vector2 position;
    Vector2 velocity;
    float radius;
    Color color;

private:
    struct TrailPoint {
        Vector2 pos;
        float life;
    };

    std::vector<TrailPoint> trail_;
};

// =====================
// Paddle
// =====================
class Paddle {
public:
    Paddle(Vector2 pos = {0, 0}, float w = 100.0f, float h = 10.0f, Color c = BLUE);
    ~Paddle();

    void Update(float dt);
    void Draw() const;
    void Extend(float extraWidth, float duration);

    Vector2 position;
    float width;
    float height;
    float originalWidth;
    float extendTimer;
    Color color;
};

// =====================
// Brick
// =====================
class Brick {
public:
    Brick(Vector2 pos = {0, 0}, float w = 0.0f, float h = 0.0f, Color c = GRAY);
    ~Brick();

    void Draw() const;

    Rectangle rect;
    bool active;
    Color color;
};

struct LevelData {
    std::vector<Brick> bricks;
    std::string backgroundTexturePath;
    std::string brickTexturePath;
    std::string hitSoundPath;
};

// =====================
// PowerUp
// =====================
class PowerUp {
public:
    PowerUp(Vector2 pos = {0, 0}, PowerUpType t = PowerUpType::PADDLE_EXTEND);
    ~PowerUp();

    void Update(float dt, float screenHeight);
    void Draw();

    Vector2 position;
    PowerUpType type;
    bool active;
    float duration;
};

// =====================
// Particle
// =====================
class Particle {
public:
    Particle(Vector2 p = {0, 0}, Vector2 v = {0, 0}, Color c = WHITE, float l = 0.0f);
    ~Particle();

    void Update(float dt);
    void Draw();

    Vector2 pos;
    Vector2 vel;
    Color color;
    float life;
};

// =====================
// PowerUpEffect
// =====================
class PowerUpEffect {
public:
    virtual ~PowerUpEffect() = default;
    virtual void Apply(Game& game) = 0;
};

class ExtendPaddleEffect : public PowerUpEffect {
public:
    ExtendPaddleEffect(float w, float d);
    ~ExtendPaddleEffect() override;

    void Apply(Game& game) override;

private:
    float extraWidth;
    float duration;
};

class MultiBallEffect : public PowerUpEffect {
public:
    MultiBallEffect(int b);
    ~MultiBallEffect() override;

    void Apply(Game& game) override;

private:
    int extraBalls;
};

class SlowBallEffect : public PowerUpEffect {
public:
    SlowBallEffect(float f, float d);
    ~SlowBallEffect() override;

    void Apply(Game& game) override;

private:
    float speedFactor;
    float duration;
};

// =====================
// Game 主类
// =====================
class Game {
public:
    Game();
    ~Game();

    void Run();

    void ApplyPaddleExtend(float extraWidth, float duration);
    void ApplySlowBall(float speedFactor, float duration);
    void AddBall(const Ball& ball);
    Ball GetBall() const;

private:
    void Update();
    void UpdateMenu();
    void UpdatePlaying();
    void UpdatePaused();
    void ResetGame();

    void Draw();
    void DrawUI();
    void DrawLoadingScreen();

    void LoadConfig();
    void LoadLeaderboard();
    void SaveScore();
    void StartLevelLoad(int level);
    LevelData BuildLevelData(int level);
    void ApplyLoadedLevel(const LevelData& data, int level);
    Color SimulateHeavyLoad();
    void ApplyLoadedBrickColor(Color c);

    bool IsButtonClicked(const Rectangle& btn);
    void ClampPaddle(Paddle& paddle);

private:
    int screenWidth_;
    int screenHeight_;
    GameState state_;
    int score_;
    int lives_;
    int currentLevel_;
    int maxLevel_;

    // 阶段1重构核心
    Paddle paddle1_;
    Paddle paddle2_;
    Ball ball_;
    std::vector<Brick> bricks_;

    // 辅助系统
    std::vector<PowerUp> powerUps_;
    std::vector<Particle> particles_;
    std::vector<std::pair<std::string, int>> leaderboardData_;
    std::vector<Vector2> originalVelocities_;

    // 配置
    json config_;
    std::string username_;
    bool isLoading_;
    std::future<LevelData> loadFuture_;
    std::atomic<bool> loadReady_;
    std::atomic<bool> loadFailed_;
    std::string loadingMessage_;
    LoadState loadState_;
    std::mutex loadMutex_;
    bool bricksColorChanged_;
    float loadCompleteTimer_;
    std::future<Color> loadColorFuture_;
    Color loadedBrickColor_;
    Texture2D backgroundTexture_{};
    Texture2D brickTexture_{};
    Sound hitSound_{};

    // 计时器
    float slowBallTimer_;
    float ballRespawnTimer_;

    // UI
    Rectangle btnPlay_;
    Rectangle btnSettings_;
    Rectangle btnQuit_;
};