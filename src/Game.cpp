#include "Game.h"

#include <fstream>
#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <memory>
#include <vector>
#include <deque>
#include <cmath>
#include <chrono>

// ============================================================
// ScoreCalculator
// ============================================================

int ScoreCalculator::CalculateBaseScore(BrickType brickType) const noexcept {
    switch (brickType) {
        case BrickType::Normal: return 10;
        case BrickType::Gold:   return 20;
        case BrickType::Bomb:   return -5;
        default:                return 0;
    }
}

int ScoreCalculator::CalculateScore(BrickType brickType, int combo) const noexcept {
    return CalculateBaseScore(brickType) + combo * comboBonus_;
}

// ============================================================
// Ball
// ============================================================

Ball::Ball(Vector2 pos, Vector2 vel, float r, Color c)
    : position(pos), velocity(vel), radius(r), color(c), trail_() {}

Ball::~Ball() = default;

void Ball::Update(float dt) {
    trail_.push_back({position, 0.3f});
    if (trail_.size() > 10) trail_.erase(trail_.begin());
    for (auto& t : trail_) t.life -= dt;

    position.x += velocity.x * dt * 60.0f;
    position.y += velocity.y * dt * 60.0f;
}

void Ball::Draw() const {
    for (size_t i = 0; i < trail_.size(); ++i) {
        float alpha = trail_[i].life / 0.3f;
        if (alpha < 0.0f) alpha = 0.0f;
        if (alpha > 1.0f) alpha = 1.0f;

        Color trailColor = Fade(color, alpha * 0.5f);
        DrawCircleV(trail_[i].pos, radius * (0.5f + i * 0.05f), trailColor);
    }
    DrawCircleV(position, radius, color);
}

// ============================================================
// Paddle
// ============================================================

Paddle::Paddle(Vector2 pos, float w, float h, Color c)
    : position(pos), width(w), height(h), originalWidth(w), extendTimer(0.0f), color(c) {}

Paddle::~Paddle() = default;

void Paddle::Extend(float extraWidth, float duration) {
    // 避免连续吃到扩展道具导致 width 叠加越来越大
    width = originalWidth + extraWidth;
    extendTimer = std::max(extendTimer, duration);
}

void Paddle::Update(float dt) {
    if (extendTimer > 0.0f) {
        extendTimer -= dt;
        if (extendTimer <= 0.0f) {
            width = originalWidth;
        }
    }
}

void Paddle::Draw() const {
    DrawRectangleRec({position.x, position.y, width, height}, color);

    if (width > originalWidth) {
        DrawRectangleLinesEx({position.x - 2, position.y - 2, width + 4, height + 4}, 2, Fade(GREEN, 0.3f));
    }
}

// ============================================================
// Brick
// ============================================================

Brick::Brick(Vector2 pos, float w, float h, Color c)
    : rect({pos.x, pos.y, w, h}), active(true), color(c) {}

Brick::~Brick() = default;

void Brick::Draw() const {
    if (active) {
        DrawRectangleRec(rect, color);
        DrawRectangleLinesEx(rect, 1, Fade(BLACK, 0.3f));
    }
}

// ============================================================
// PowerUp
// ============================================================

PowerUp::PowerUp(Vector2 pos, PowerUpType t)
    : position(pos), type(t), active(true), duration(5.0f) {}

PowerUp::~PowerUp() = default;

void PowerUp::Update(float dt, float screenHeight) {
    position.y += 100.0f * dt;
    if (position.y > screenHeight) active = false;
}

void PowerUp::Draw() {
    if (!active) return;

    Color color = (type == PowerUpType::PADDLE_EXTEND) ? GREEN :
                  (type == PowerUpType::MULTI_BALL) ? BLUE : YELLOW;

    DrawRectangleRec({position.x - 10, position.y - 5, 20, 10}, color);
    DrawCircleV(position, 8, color);

    float rotation = (float)GetTime() * 2.0f;
    DrawCircleSector(position, 12, rotation, rotation + 180, 3, Fade(color, 0.3f));
}

// ============================================================
// Particle
// ============================================================

Particle::Particle(Vector2 p, Vector2 v, Color c, float l)
    : pos(p), vel(v), color(c), life(l) {}

Particle::~Particle() = default;

void Particle::Update(float dt) {
    pos.x += vel.x * dt;
    pos.y += vel.y * dt;
    life -= dt;
}

void Particle::Draw() {
    if (life > 0.0f) {
        float alpha = life / 0.5f;
        if (alpha < 0.0f) alpha = 0.0f;
        if (alpha > 1.0f) alpha = 1.0f;
        DrawCircleV(pos, 2, Fade(color, alpha));
    }
}

// ============================================================
// PowerUpEffect
// ============================================================

ExtendPaddleEffect::ExtendPaddleEffect(float w, float d)
    : extraWidth(w), duration(d) {}

ExtendPaddleEffect::~ExtendPaddleEffect() = default;

void ExtendPaddleEffect::Apply(Game& game) {
    game.ApplyPaddleExtend(extraWidth, duration);
}

MultiBallEffect::MultiBallEffect(int b)
    : extraBalls(b) {}

MultiBallEffect::~MultiBallEffect() = default;

void MultiBallEffect::Apply(Game& game) {
    // 阶段1：单球结构下先保留接口，不再真正生成多球
    // 如果你后面还想恢复多球，可以再把 Game 改回 vector<Ball>
    (void)game;
}

SlowBallEffect::SlowBallEffect(float f, float d)
    : speedFactor(f), duration(d) {}

SlowBallEffect::~SlowBallEffect() = default;

void SlowBallEffect::Apply(Game& game) {
    game.ApplySlowBall(speedFactor, duration);
}

// ============================================================
// PowerUpEffect factory
// ============================================================

static std::unique_ptr<PowerUpEffect> CreatePowerUpEffect(PowerUpType type, const json& config) {
    const float fixedDuration = 5.0f;
    float extraWidth = 40.0f;
    int extraBalls = 2;
    float speedFactor = 0.7f;

    if (config.contains("powerups")) {
        const json& powerups = config["powerups"];
        if (powerups.contains("paddle_extend")) {
            extraWidth = powerups["paddle_extend"].value("extra_width", extraWidth);
        }
        if (powerups.contains("multi_ball")) {
            extraBalls = powerups["multi_ball"].value("extra_balls", extraBalls);
        }
        if (powerups.contains("slow_ball")) {
            speedFactor = powerups["slow_ball"].value("speed_factor", speedFactor);
        }
    }

    switch (type) {
        case PowerUpType::PADDLE_EXTEND:
            return std::make_unique<ExtendPaddleEffect>(extraWidth, fixedDuration);
        case PowerUpType::MULTI_BALL:
            return std::make_unique<MultiBallEffect>(extraBalls);
        case PowerUpType::SLOW_BALL:
            return std::make_unique<SlowBallEffect>(speedFactor, fixedDuration);
        default:
            return nullptr;
    }
}

// ============================================================
// Game
// ============================================================

Game::Game()
    : screenWidth_(800),
      screenHeight_(600),
      state_(GameState::MENU),
      score_(0),
      lives_(5),
      currentLevel_(1),
      maxLevel_(3),
      isLoading_(false),
      loadReady_(false),
      loadFailed_(false),
      loadState_(LoadState::IDLE),
      bricksColorChanged_(false),
      loadCompleteTimer_(0.0f),
      loadedBrickColor_(GOLD),
      paddle1_(Vector2{250.0f, 550.0f}, 100.0f, 10.0f, BLUE),
      paddle2_(Vector2{450.0f, 550.0f}, 100.0f, 10.0f, GREEN),
      ball_(Vector2{400.0f, 300.0f}, Vector2{4.0f, -4.0f}, 10.0f, RED),
      slowBallTimer_(0.0f),
      ballRespawnTimer_(0.0f) {
    srand((unsigned)time(nullptr));

    LoadConfig();

    // 配置文件可能缺字段：这里用默认值读取，避免直接抛异常崩溃
    screenWidth_ = config_.value("screen", json::object()).value("width", screenWidth_);
    screenHeight_ = config_.value("screen", json::object()).value("height", screenHeight_);

    const std::string title =
        config_.value("screen", json::object()).value("title", std::string("Breakout Game"));
    InitWindow(screenWidth_, screenHeight_, title.c_str());
    InitAudioDevice();
    SetTargetFPS(60);
    SetExitKey(0);

    int btnW = 200, btnH = 40;
    int startX = screenWidth_ / 2 - btnW / 2;
    int startY = 200;

    btnPlay_ = { (float)startX, (float)startY, (float)btnW, (float)btnH };
    btnSettings_ = { (float)startX, (float)startY + 60, (float)btnW, (float)btnH };
    btnQuit_ = { (float)startX, (float)startY + 120, (float)btnW, (float)btnH };

    LoadLeaderboard();
}

Game::~Game() {
    if (backgroundTexture_.id != 0) UnloadTexture(backgroundTexture_);
    if (brickTexture_.id != 0) UnloadTexture(brickTexture_);
    if (hitSound_.frameCount > 0) UnloadSound(hitSound_);
    if (IsAudioDeviceReady()) CloseAudioDevice();
    CloseWindow();
}

void Game::Run() {
    while (!WindowShouldClose()) {
        Update();
        Draw();
    }
}

// ============================================================
// Update
// ============================================================

void Game::Update() {
    switch (state_) {
        case GameState::MENU:
            UpdateMenu();
            break;
        case GameState::PLAYING:
            UpdatePlaying();
            break;
        case GameState::PAUSED:
            UpdatePaused();
            break;
        case GameState::GAME_OVER:
            if (IsKeyPressed(KEY_ENTER)) state_ = GameState::MENU;
            break;
        case GameState::LEADERBOARD:
            if (IsKeyPressed(KEY_ESCAPE)) state_ = GameState::MENU;
            break;
    }
}

void Game::UpdateMenu() {
    if (IsButtonClicked(btnPlay_)) {
        state_ = GameState::PLAYING;
        ResetGame();
    }
    if (IsButtonClicked(btnSettings_)) state_ = GameState::LEADERBOARD;
    if (IsButtonClicked(btnQuit_)) exit(0);
}

void Game::UpdatePaused() {
    if (IsKeyPressed(KEY_C)) state_ = GameState::PLAYING;
    if (IsKeyPressed(KEY_Q)) state_ = GameState::MENU;
}

void Game::ClampPaddle(Paddle& paddle) {
    if (paddle.position.x < 0.0f) paddle.position.x = 0.0f;
    if (paddle.position.x + paddle.width > screenWidth_)
        paddle.position.x = screenWidth_ - paddle.width;

    if (paddle.position.y < 0.0f) paddle.position.y = 0.0f;
    if (paddle.position.y + paddle.height > screenHeight_)
        paddle.position.y = screenHeight_ - paddle.height;
}

void Game::UpdatePlaying() {
    float dt = GetFrameTime();

    // L 键：触发异步模拟加载
    if (IsKeyPressed(KEY_L)) {
        std::lock_guard<std::mutex> lock(loadMutex_);
        if (loadState_ == LoadState::IDLE) {
            loadState_ = LoadState::LOADING;
            bricksColorChanged_ = false;
            loadCompleteTimer_ = 0.0f;
            loadedBrickColor_ = GOLD;

            loadColorFuture_ = std::async(std::launch::async, [this]() {
                return SimulateHeavyLoad();
            });
        }
    }

    {
        std::lock_guard<std::mutex> lock(loadMutex_);
        if (loadState_ == LoadState::LOADING) {
            auto status = loadColorFuture_.wait_for(std::chrono::seconds(0));
            if (status == std::future_status::ready) {
                try {
                    Color c = loadColorFuture_.get();
                    ApplyLoadedBrickColor(c);
                    loadedBrickColor_ = c;
                    loadState_ = LoadState::DONE;
                    loadCompleteTimer_ = 2.0f;
                } catch (...) {
                    loadState_ = LoadState::IDLE;
                }
            }
        }
    }

    if (isLoading_) {
        auto status = loadFuture_.wait_for(std::chrono::milliseconds(0));
        if (status == std::future_status::ready) {
            try {
                LevelData data = loadFuture_.get();
                int targetLevel = currentLevel_ == 1 && bricks_.empty() ? 1 : currentLevel_ + 1;
                ApplyLoadedLevel(data, targetLevel);
            } catch (...) {
                isLoading_ = false;
                loadFailed_ = true;
                loadingMessage_ = "Load Failed!";
            }
        }
        return;
    }

    if (loadFailed_) {
        return;
    }

    // 更新挡板
    paddle1_.Update(dt);
    paddle2_.Update(dt);

    // 更新慢球计时
    if (slowBallTimer_ > 0.0f) {
        slowBallTimer_ -= dt;
        if (slowBallTimer_ <= 0.0f && !originalVelocities_.empty()) {
            ball_.velocity = originalVelocities_[0];
            originalVelocities_.clear();
        }
    }

    // 更新道具
    for (auto& p : powerUps_) {
        p.Update(dt, (float)screenHeight_);
    }
    powerUps_.erase(
        std::remove_if(powerUps_.begin(), powerUps_.end(),
                       [](const PowerUp& p) { return !p.active; }),
        powerUps_.end()
    );

    // 更新粒子
    for (auto& part : particles_) {
        part.Update(dt);
    }
    particles_.erase(
        std::remove_if(particles_.begin(), particles_.end(),
                       [](const Particle& part) { return part.life <= 0.0f; }),
        particles_.end()
    );

    // 双挡板控制
    float paddleSpeed = 500.0f * dt;

    // paddle1_：WASD
    if (IsKeyDown(KEY_W)) paddle1_.position.y -= paddleSpeed;
    if (IsKeyDown(KEY_A)) paddle1_.position.x -= paddleSpeed;
    if (IsKeyDown(KEY_S)) paddle1_.position.y += paddleSpeed;
    if (IsKeyDown(KEY_D)) paddle1_.position.x += paddleSpeed;

    // paddle2_：方向键
    if (IsKeyDown(KEY_UP))    paddle2_.position.y -= paddleSpeed;
    if (IsKeyDown(KEY_LEFT))  paddle2_.position.x -= paddleSpeed;
    if (IsKeyDown(KEY_DOWN))  paddle2_.position.y += paddleSpeed;
    if (IsKeyDown(KEY_RIGHT)) paddle2_.position.x += paddleSpeed;

    ClampPaddle(paddle1_);
    ClampPaddle(paddle2_);

    // 球重生计时：倒计时期间允许移动/道具/粒子继续更新，但不更新球物理与碰撞
    if (ballRespawnTimer_ > 0.0f) {
        ballRespawnTimer_ -= dt;
        if (ballRespawnTimer_ <= 0.0f) {
            float speed = config_["ball"]["speed_base"].get<float>();
            ball_ = Ball(
                { screenWidth_ / 2.0f, screenHeight_ / 2.0f },
                { speed, -speed },
                config_["ball"]["radius"].get<float>(),
                RED
            );
        }
        if (IsKeyPressed(KEY_ESCAPE)) state_ = GameState::PAUSED;
        return;
    }

    // 更新球
    ball_.Update(dt);

    // 屏幕边界反弹
    if (ball_.position.x - ball_.radius <= 0.0f) {
        ball_.position.x = ball_.radius;
        ball_.velocity.x *= -1.0f;
    } else if (ball_.position.x + ball_.radius >= screenWidth_) {
        ball_.position.x = (float)screenWidth_ - ball_.radius;
        ball_.velocity.x *= -1.0f;
    }

    if (ball_.position.y - ball_.radius <= 0.0f) {
        ball_.position.y = ball_.radius;
        ball_.velocity.y *= -1.0f;
    }

    // 挡板矩形
    Rectangle paddleRect1 = { paddle1_.position.x, paddle1_.position.y, paddle1_.width, paddle1_.height };
    Rectangle paddleRect2 = { paddle2_.position.x, paddle2_.position.y, paddle2_.width, paddle2_.height };

    // 球与挡板碰撞
    if (CheckCollisionCircleRec(ball_.position, ball_.radius, paddleRect1) && ball_.velocity.y > 0.0f) {
        ball_.velocity.y *= -1.0f;
        float hitPos = (ball_.position.x - paddle1_.position.x) / paddle1_.width;
        ball_.velocity.x = 8.0f * (hitPos - 0.5f);
    }

    if (CheckCollisionCircleRec(ball_.position, ball_.radius, paddleRect2) && ball_.velocity.y > 0.0f) {
        ball_.velocity.y *= -1.0f;
        float hitPos = (ball_.position.x - paddle2_.position.x) / paddle2_.width;
        ball_.velocity.x = 8.0f * (hitPos - 0.5f);
    }

    // 球与砖块碰撞
    for (auto& brick : bricks_) {
        if (brick.active && CheckCollisionCircleRec(ball_.position, ball_.radius, brick.rect)) {
            brick.active = false;
            ball_.velocity.y *= -1.0f;
            score_ += 1;

            if (IsAudioDeviceReady() && hitSound_.frameCount > 0) {
                PlaySound(hitSound_);
            }

            // 粒子效果
            for (int j = 0; j < 20; ++j) {
                particles_.push_back(Particle(
                    Vector2{
                        brick.rect.x + (float)(rand() % (int)brick.rect.width),
                        brick.rect.y + (float)(rand() % (int)brick.rect.height)
                    },
                    Vector2{
                        (float)((rand() % 200 - 100) / 10.0f),
                        (float)((rand() % 200 - 100) / 10.0f)
                    },
                    brick.color,
                    0.5f
                ));
            }

            // 道具掉落
            if ((rand() % 100) < 30) {
                PowerUpType type = static_cast<PowerUpType>(rand() % 3);
                PowerUp powerUp({ brick.rect.x + brick.rect.width / 2, brick.rect.y }, type);
                powerUp.duration = 5.0f;
                powerUps_.push_back(powerUp);
            }

            break;
        }
    }

    // 道具接住
    for (auto& powerUp : powerUps_) {
        if (!powerUp.active) continue;

        Rectangle powerUpRect = { powerUp.position.x - 10, powerUp.position.y - 5, 20, 10 };

        if (CheckCollisionRecs(paddleRect1, powerUpRect) || CheckCollisionRecs(paddleRect2, powerUpRect)) {
            powerUp.active = false;
            auto effect = CreatePowerUpEffect(powerUp.type, config_);
            if (effect) effect->Apply(*this);
        }
    }

    // 掉出屏幕下方
    if (ball_.position.y + ball_.radius >= screenHeight_) {
        lives_--;
        if (lives_ <= 0) {
            state_ = GameState::GAME_OVER;
            SaveScore();
        } else {
            // 保留极短重生缓冲，避免看起来像整局“卡住”
            ballRespawnTimer_ = 0.35f;
            // 立刻把球“收回”，避免连续多帧反复扣命
            ball_.position = { screenWidth_ / 2.0f, screenHeight_ / 2.0f };
            ball_.velocity = { 0.0f, 0.0f };
        }
    }

    // 胜利条件：砖块全清
    bool allBricksDestroyed = std::all_of(bricks_.begin(), bricks_.end(),
        [](const Brick& b) { return !b.active; });

    if (allBricksDestroyed) {
        if (currentLevel_ < maxLevel_) {
            StartLevelLoad(currentLevel_ + 1);
        } else {
            state_ = GameState::GAME_OVER;
            SaveScore();
        }
    }

    if (loadCompleteTimer_ > 0.0f) {
        loadCompleteTimer_ -= dt;
        if (loadCompleteTimer_ <= 0.0f) {
            std::lock_guard<std::mutex> lock(loadMutex_);
            loadState_ = LoadState::IDLE;
            loadCompleteTimer_ = 0.0f;
        }
    }

    if (IsKeyPressed(KEY_ESCAPE)) state_ = GameState::PAUSED;
}

// ============================================================
// Reset
// ============================================================

void Game::ResetGame() {
    lives_ = config_["game"]["max_lives"].get<int>();
    score_ = 0;

    float speed = config_["ball"]["speed_base"].get<float>();
    float radius = config_["ball"]["radius"].get<float>();

    ball_ = Ball(
        { screenWidth_ / 2.0f, screenHeight_ / 2.0f },
        { speed, -speed },
        radius,
        RED
    );

    float paddleWidth = config_["paddle"]["width"].get<float>();
    float paddleHeight = config_["paddle"]["height"].get<float>();

    paddle1_.width = paddleWidth;
    paddle1_.height = paddleHeight;
    paddle1_.originalWidth = paddleWidth;
    paddle1_.position = { screenWidth_ * 0.25f - paddleWidth / 2.0f, (float)screenHeight_ - 50 };
    paddle1_.color = BLUE;
    paddle1_.extendTimer = 0.0f;

    paddle2_.width = paddleWidth;
    paddle2_.height = paddleHeight;
    paddle2_.originalWidth = paddleWidth;
    paddle2_.position = { screenWidth_ * 0.75f - paddleWidth / 2.0f, (float)screenHeight_ - 50 };
    paddle2_.color = GREEN;
    paddle2_.extendTimer = 0.0f;

    bricks_.clear();

    powerUps_.clear();
    particles_.clear();
    slowBallTimer_ = 0.0f;
    ballRespawnTimer_ = 0.0f;
    originalVelocities_.clear();
    currentLevel_ = 1;
    isLoading_ = false;
    loadReady_ = false;
    loadFailed_ = false;
    loadingMessage_.clear();

    StartLevelLoad(1);
}

// ============================================================
// PowerUp effects
// ============================================================

void Game::ApplyPaddleExtend(float extraWidth, float duration) {
    // 阶段1建议：两个挡板都扩展，便于观察效果是否正常
    paddle1_.Extend(extraWidth, duration);
    paddle2_.Extend(extraWidth, duration);
}

void Game::ApplySlowBall(float speedFactor, float duration) {
    originalVelocities_.clear();
    originalVelocities_.push_back(ball_.velocity);

    ball_.velocity.x *= speedFactor;
    ball_.velocity.y *= speedFactor;

    slowBallTimer_ = duration;
}

void Game::AddBall(const Ball& ball) {
    // 阶段1单球结构下，保留接口但不真正扩展多球系统
    (void)ball;
}

Ball Game::GetBall() const {
    return ball_;
}

// ============================================================
// 配置/排行榜
// ============================================================

void Game::LoadConfig() {
    std::ifstream file("config.json");
    if (file.is_open()) {
        file >> config_;
    } else {
        config_ = {
            {"screen", {{"width", 800}, {"height", 600}, {"title", "Breakout Game"}}},
            {"paddle", {{"width", 100.0}, {"height", 10.0}, {"speed", 7.0}, {"color", {0, 0, 139, 255}}}},
            {"ball", {{"radius", 10.0}, {"speed_base", 4.0}, {"color", {139, 0, 0, 255}}}},
            {"bricks", {{"rows", 5}, {"cols", 10}, {"width", 75}, {"height", 20}, {"spacing", 5}, {"offset_x", 25}, {"offset_y", 50}}},
            {"game", {{"max_lives", 5}}},
            {"powerups", {
                {"paddle_extend", {{"extra_width", 40}, {"duration", 5}, {"drop_rate", 0.3}}},
                {"multi_ball", {{"extra_balls", 2}, {"duration", 0}, {"drop_rate", 0.2}}},
                {"slow_ball", {{"speed_factor", 0.7}, {"duration", 5}, {"drop_rate", 0.25}}}
            }}
        };
    }
}

void Game::LoadLeaderboard() {
    leaderboardData_.clear();

    std::ifstream file("leaderboard.txt");
    if (!file.is_open()) return;

    std::string line;
    while (std::getline(file, line)) {
        size_t pos = line.find(' ');
        if (pos != std::string::npos) {
            std::string name = line.substr(0, pos);
            int sc = std::stoi(line.substr(pos + 1));
            leaderboardData_.emplace_back(name, sc);
        }
    }

    std::sort(leaderboardData_.begin(), leaderboardData_.end(),
              [](auto& a, auto& b) { return a.second > b.second; });
}

void Game::SaveScore() {
    leaderboardData_.push_back({username_, score_});
    std::sort(leaderboardData_.begin(), leaderboardData_.end(),
              [](auto& a, auto& b) { return a.second > b.second; });

    std::ofstream file("leaderboard.txt");
    if (!file.is_open()) return;

    for (auto& p : leaderboardData_) {
        file << p.first << " " << p.second << "\n";
    }
}

// ============================================================
// UI / Draw
// ============================================================

void Game::DrawUI() {
    DrawText(TextFormat("Score: %d", score_), 10, 10, 20, BLACK);
    DrawText(TextFormat("Lives: %d", lives_), 10, 40, 20, BLACK);
    DrawText(TextFormat("Level: %d/%d", currentLevel_, maxLevel_), 10, 70, 20, BLACK);

    if (paddle1_.extendTimer > 0.0f) {
        DrawText(TextFormat("P1 Extend: %.1f", paddle1_.extendTimer), 10, 100, 20, BLUE);
    }
    if (paddle2_.extendTimer > 0.0f) {
        DrawText(TextFormat("P2 Extend: %.1f", paddle2_.extendTimer), 10, 130, 20, GREEN);
    }
    if (slowBallTimer_ > 0.0f) {
        DrawText(TextFormat("Slow Ball: %.1f", slowBallTimer_), 10, 160, 20, YELLOW);
    }
}

void Game::Draw() {
    BeginDrawing();
    ClearBackground(RAYWHITE);

    switch (state_) {
        case GameState::MENU:
            DrawRectangle(0, 0, screenWidth_, screenHeight_, Fade(BLACK, 0.3f));
            DrawText("BREAKOUT", screenWidth_ / 2 - 80, 100, 40, BLACK);

            Color color;
            for (auto& btn : {&btnPlay_, &btnSettings_, &btnQuit_}) {
                bool hover = CheckCollisionPointRec(GetMousePosition(), *btn);
                float scale = hover ? 1.1f : 1.0f;
                color = hover ? Color{255, 100, 100, 255} : LIGHTGRAY;
                if (btn == &btnSettings_) color = hover ? Color{100, 255, 100, 255} : LIGHTGRAY;
                if (btn == &btnQuit_) color = hover ? Color{100, 100, 255, 255} : LIGHTGRAY;

                Rectangle scaledRect = {
                    btn->x - btn->width * (scale - 1) / 2,
                    btn->y - btn->height * (scale - 1) / 2,
                    btn->width * scale,
                    btn->height * scale
                };

                DrawRectangleRec(scaledRect, color);
                const char* txt = (btn == &btnPlay_) ? "PLAY" :
                                  (btn == &btnSettings_) ? "SETTINGS" : "QUIT";
                int xOffset = (btn == &btnPlay_) ? 70 : (btn == &btnSettings_) ? 55 : 75;
                DrawText(txt,
                         (int)(scaledRect.x + xOffset * scale),
                         (int)(scaledRect.y + 10 * scale),
                         (int)(20 * scale),
                         BLACK);
            }
            break;

        case GameState::PLAYING:
            {
                bool showColorLoad = false;
                {
                    std::lock_guard<std::mutex> lock(loadMutex_);
                    showColorLoad = (loadState_ == LoadState::LOADING);
                }

                if (isLoading_ || loadFailed_ || showColorLoad) {
                    DrawLoadingScreen();
                } else {
                    if (backgroundTexture_.id != 0) {
                        DrawTexturePro(
                            backgroundTexture_,
                            Rectangle{0.0f, 0.0f, (float)backgroundTexture_.width, (float)backgroundTexture_.height},
                            Rectangle{0.0f, 0.0f, (float)screenWidth_, (float)screenHeight_},
                            Vector2{0.0f, 0.0f},
                            0.0f,
                            WHITE
                        );
                    }

                    ball_.Draw();
                    paddle1_.Draw();
                    paddle2_.Draw();

                    for (auto& brick : bricks_) {
                        if (!brick.active) continue;

                        if (brickTexture_.id != 0) {
                            DrawTexturePro(
                                brickTexture_,
                                Rectangle{0.0f, 0.0f, (float)brickTexture_.width, (float)brickTexture_.height},
                                brick.rect,
                                Vector2{0.0f, 0.0f},
                                0.0f,
                                WHITE
                            );
                            DrawRectangleLinesEx(brick.rect, 1, Fade(BLACK, 0.25f));
                        } else {
                            brick.Draw();
                        }
                    }

                    for (auto& p : powerUps_) p.Draw();
                    for (auto& part : particles_) part.Draw();
                    DrawUI();

                    if (loadCompleteTimer_ > 0.0f) {
                        const char* msg = "Load Complete!";
                        int w = MeasureText(msg, 25);
                        DrawRectangle(screenWidth_ / 2 - w / 2 - 10, 30, w + 20, 40, Fade(GOLD, 0.7f));
                        DrawText(msg, screenWidth_ / 2 - w / 2, 40, 25, BLACK);
                    }
                }
            }
            break;

        case GameState::PAUSED:
            if (backgroundTexture_.id != 0) {
                DrawTexturePro(
                    backgroundTexture_,
                    Rectangle{0.0f, 0.0f, (float)backgroundTexture_.width, (float)backgroundTexture_.height},
                    Rectangle{0.0f, 0.0f, (float)screenWidth_, (float)screenHeight_},
                    Vector2{0.0f, 0.0f},
                    0.0f,
                    WHITE
                );
            }
            ball_.Draw();
            paddle1_.Draw();
            paddle2_.Draw();
            for (auto& brick : bricks_) {
                if (!brick.active) continue;

                if (brickTexture_.id != 0) {
                    DrawTexturePro(
                        brickTexture_,
                        Rectangle{0.0f, 0.0f, (float)brickTexture_.width, (float)brickTexture_.height},
                        brick.rect,
                        Vector2{0.0f, 0.0f},
                        0.0f,
                        WHITE
                    );
                    DrawRectangleLinesEx(brick.rect, 1, Fade(BLACK, 0.25f));
                } else {
                    brick.Draw();
                }
            }
            for (auto& p : powerUps_) p.Draw();
            for (auto& part : particles_) part.Draw();
            DrawUI();
            DrawRectangle(0, 0, screenWidth_, screenHeight_, {0, 0, 0, 150});
            DrawRectangle(screenWidth_ / 2 - 100, screenHeight_ / 2 - 50, 200, 100, LIGHTGRAY);
            DrawText("PAUSED", screenWidth_ / 2 - 40, screenHeight_ / 2 - 40, 20, BLACK);
            DrawText("Continue (C) / Quit (Q)", screenWidth_ / 2 - 100, screenHeight_ / 2, 15, BLACK);
            break;

        case GameState::GAME_OVER: {
            DrawText("GAME OVER", screenWidth_ / 2 - 80, 200, 40, RED);
            DrawText(TextFormat("Score: %d", score_), screenWidth_ / 2 - 80, 260, 20, BLACK);

            int rank = 1;
            for (auto& p : leaderboardData_) {
                if (p.second > score_) rank++;
            }

            DrawText(TextFormat("Rank: %d", rank), screenWidth_ / 2 - 80, 300, 20, BLACK);
            DrawText("Press ENTER to Menu", screenWidth_ / 2 - 100, 400, 20, GRAY);
            break;
        }

        case GameState::LEADERBOARD: {
            DrawRectangle(0, 0, screenWidth_, screenHeight_, Fade(BLACK, 0.3f));
            DrawText("LEADERBOARD", screenWidth_ / 2 - 80, 100, 30, BLACK);

            int y = 150;
            for (size_t i = 0; i < leaderboardData_.size() && i < 10; i++) {
                DrawText(
                    TextFormat("%d. %s - %d", (int)i + 1,
                               leaderboardData_[i].first.c_str(),
                               leaderboardData_[i].second),
                    screenWidth_ / 2 - 100,
                    y,
                    20,
                    BLACK
                );
                y += 30;
            }

            DrawText("Press ESC to return", screenWidth_ / 2 - 80, 400, 20, GRAY);
            break;
        }
    }

    EndDrawing();
}

// ============================================================
// Misc
// ============================================================

bool Game::IsButtonClicked(const Rectangle& btn) {
    return IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(GetMousePosition(), btn);
}

void Game::StartLevelLoad(int level) {
    if (level > maxLevel_) {
        state_ = GameState::GAME_OVER;
        SaveScore();
        return;
    }

    if (isLoading_) return;

    isLoading_ = true;
    loadReady_ = false;
    loadFailed_ = false;
    loadingMessage_ = "Loading";

    loadFuture_ = std::async(std::launch::async, [this, level]() {
        return BuildLevelData(level);
    });
}

LevelData Game::BuildLevelData(int level) {
    LevelData data;

    float bWidth = config_["bricks"]["width"].get<float>();
    float bHeight = config_["bricks"]["height"].get<float>();
    float spacing = config_["bricks"]["spacing"].get<float>();

    switch (level) {
        case 1: {
            data.backgroundTexturePath = "assets/bg_level1.png";
            data.brickTexturePath = "assets/brick_level1.png";
            data.hitSoundPath = "assets/hit1.wav";

            int rows = 4;
            int cols = 8;
            float offsetX = 60;
            float offsetY = 60;

            data.bricks.clear();
            for (int r = 0; r < rows; ++r) {
                for (int c = 0; c < cols; ++c) {
                    data.bricks.emplace_back(
                        Vector2{offsetX + c * (bWidth + spacing), offsetY + r * (bHeight + spacing)},
                        bWidth, bHeight,
                        ORANGE
                    );
                }
            }
            break;
        }

        case 2: {
            data.backgroundTexturePath = "assets/bg_level2.png";
            data.brickTexturePath = "assets/brick_level2.png";
            data.hitSoundPath = "assets/hit2.wav";

            int rows = 5;
            int cols = 10;
            float offsetX = 30;
            float offsetY = 50;

            data.bricks.clear();
            for (int r = 0; r < rows; ++r) {
                for (int c = 0; c < cols; ++c) {
                    if (r == 2 && (c == 4 || c == 5)) continue;

                    data.bricks.emplace_back(
                        Vector2{offsetX + c * (bWidth + spacing), offsetY + r * (bHeight + spacing)},
                        bWidth, bHeight,
                        SKYBLUE
                    );
                }
            }
            break;
        }

        case 3: {
            data.backgroundTexturePath = "assets/bg_level3.png";
            data.brickTexturePath = "assets/brick_level3.png";
            data.hitSoundPath = "assets/hit3.wav";

            int rows = 6;
            int cols = 12;
            float offsetX = 20;
            float offsetY = 40;

            data.bricks.clear();
            for (int r = 0; r < rows; ++r) {
                for (int c = 0; c < cols; ++c) {
                    if (c < r || c >= cols - r) continue;

                    data.bricks.emplace_back(
                        Vector2{offsetX + c * (bWidth + spacing), offsetY + r * (bHeight + spacing)},
                        bWidth, bHeight,
                        PINK
                    );
                }
            }
            break;
        }

        default:
            break;
    }

    return data;
}

void Game::ApplyLoadedLevel(const LevelData& data, int level) {
    bricks_ = data.bricks;

    if (backgroundTexture_.id != 0) {
        UnloadTexture(backgroundTexture_);
        backgroundTexture_ = Texture2D{};
    }
    if (brickTexture_.id != 0) {
        UnloadTexture(brickTexture_);
        brickTexture_ = Texture2D{};
    }
    if (hitSound_.frameCount > 0) {
        UnloadSound(hitSound_);
        hitSound_ = Sound{};
    }

    backgroundTexture_ = LoadTexture(data.backgroundTexturePath.c_str());
    brickTexture_ = LoadTexture(data.brickTexturePath.c_str());
    hitSound_ = LoadSound(data.hitSoundPath.c_str());

    currentLevel_ = std::min(level, maxLevel_);
    loadReady_ = true;
    isLoading_ = false;
    loadingMessage_.clear();
}

void Game::DrawLoadingScreen() {
    ClearBackground(RAYWHITE);

    static float t = 0.0f;
    t += GetFrameTime();

    std::string text = loadingMessage_;
    if (text.empty()) {
        int dots = (static_cast<int>(t * 3.0f)) % 4;
        text = "Loading";
        for (int i = 0; i < dots; ++i) text += ".";
    }

    int textWidth = MeasureText(text.c_str(), 30);
    DrawText(text.c_str(), screenWidth_ / 2 - textWidth / 2, screenHeight_ / 2 - 15, 30, DARKGRAY);

    float angle = t * 180.0f;
    Vector2 center = { (float)screenWidth_ / 2.0f, (float)screenHeight_ / 2.0f + 50 };

    DrawCircleLines((int)center.x, (int)center.y, 20, LIGHTGRAY);
    DrawLineEx(
        { center.x, center.y },
        { center.x + 20.0f * cosf(angle * DEG2RAD), center.y + 20.0f * sinf(angle * DEG2RAD) },
        4,
        DARKGRAY
    );
}

Color Game::SimulateHeavyLoad() {
    std::this_thread::sleep_for(std::chrono::seconds(2));
    return GOLD;
}

void Game::ApplyLoadedBrickColor(Color c) {
    for (auto& brick : bricks_) {
        brick.color = c;
    }
    bricksColorChanged_ = true;
}