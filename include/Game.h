#ifndef GAME_H
#define GAME_H

#include "raylib.h"
#include <deque>
#include <vector>
#include <memory>
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class GameObject;

struct TrailPoint {
    Vector2 pos;
    float life;
};

class Ball {
public:
    Vector2 position;
    Vector2 velocity;
    float radius;
    Color color;
    std::deque<TrailPoint> trail;

    Ball(Vector2 pos, Vector2 vel, float r, Color c);
    ~Ball();
    void Update(float dt);
    void Draw() const;
};

class Paddle {
public:
    Vector2 position;
    float width, height;
    Color color;
    float originalWidth;
    float extendTimer;

    Paddle(Vector2 pos, float w, float h, Color c);
    ~Paddle();
    void Extend(float extraWidth, float duration);
    void Update(float dt);
    void Draw() const;
};

class Brick {
public:
    Rectangle rect;
    bool active;
    Color color;
    Brick(Vector2 pos, float w, float h, Color c);
    ~Brick();
    void Draw() const;
};

enum class PowerUpType { PADDLE_EXTEND = 0, MULTI_BALL = 1, SLOW_BALL = 2 };

class PowerUp {
public:
    Vector2 position;
    PowerUpType type;
    bool active;
    float duration;

    PowerUp(Vector2 pos, PowerUpType t);
    ~PowerUp();
    void Update(float dt, float screenHeight);
    void Draw();
};

class Particle {
public:
    Vector2 pos, vel;
    Color color;
    float life;
    Particle(Vector2 p, Vector2 v, Color c, float l);
    ~Particle();
    void Update(float dt);
    void Draw();
};

class Game;

// 道具效果基类和派生类
class PowerUpEffect {
public:
    virtual ~PowerUpEffect() {}
    virtual void Apply(Game& game) = 0;
};

class ExtendPaddleEffect : public PowerUpEffect {
    float extraWidth;
    float duration;
public:
    ExtendPaddleEffect(float w, float d);
    ~ExtendPaddleEffect();
    void Apply(Game& game) override;
};

class MultiBallEffect : public PowerUpEffect {
    int extraBalls;
public:
    MultiBallEffect(int b);
    ~MultiBallEffect();
    void Apply(Game& game) override;
};

class SlowBallEffect : public PowerUpEffect {
    float speedFactor;
    float duration;
public:
    SlowBallEffect(float f, float d);
    ~SlowBallEffect();
    void Apply(Game& game) override;
};

enum class GameState { MENU, PLAYING, PAUSED, GAME_OVER, LEADERBOARD };

class Game {
private:
    int screenWidth_, screenHeight_;
    GameState state_;
    int score_;
    int lives_;
    int level_;

    // 窗口控件
    Rectangle btnPlay_, btnSettings_, btnQuit_;
    std::string username_ = "Player";

    std::vector<std::unique_ptr<GameObject>> gameObjects_;
    GameObject* paddlePtr_ = nullptr;

    Paddle paddle_;
    std::vector<Ball> balls_;
    std::vector<Brick> bricks_;
    std::vector<PowerUp> powerUps_;
    std::vector<Particle> particles_;

    std::vector<std::pair<std::string,int>> leaderboardData_;

    json config_;

    // 速度、延时相关
    float paddleExtendTimer_;
    float slowBallTimer_;
    float ballRespawnTimer_;

    std::vector<Vector2> originalVelocities_;

public:
    Game();
    ~Game();

    void Run();
    void Update();
    void UpdateMenu();
    void UpdatePlaying();
    void UpdatePaused();
    void ResetGame();

    void Draw();
    void DrawUI();

    void ApplyPaddleExtend(float extraWidth, float duration);
    void ApplySlowBall(float speedFactor, float duration);

    void AddBall(const Ball& ball);
    Ball GetBall() const;

    void LoadConfig();
    void LoadLeaderboard();
    void SaveScore();

    void InitGameObjects();

    static bool IsButtonClicked(const Rectangle& btn);
    static Color JsonToColor(const json& j);
};

#endif // GAME_H