#include "Game.h"
#include "JsonIO.h"

#include <fstream>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <ctime>
#include <memory>
#include <vector>
#include <deque>
#include <cmath>
#include <chrono>
#include <random>

// ============================================================
// ScoreCalculator — 按砖块类型与连击计算得分
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
    : position(pos), velocity(vel), radius(r), color(c) {}

Ball::~Ball() = default;

void Ball::Update(float dt) {
    if (trailCount_ < MAX_TRAIL) {
        const int w = (trailHead_ + trailCount_) % MAX_TRAIL;
        trail_[w] = {position, 0.3f};
        ++trailCount_;
    } else {
        trail_[trailHead_] = {position, 0.3f};
        trailHead_ = (trailHead_ + 1) % MAX_TRAIL;
    }
    for (int i = 0; i < trailCount_; ++i) {
        const int idx = (trailHead_ + i) % MAX_TRAIL;
        trail_[idx].life -= dt;
    }

    position.x += velocity.x * dt * 60.0f;
    position.y += velocity.y * dt * 60.0f;
}

void Ball::Draw() const {
    for (int i = 0; i < trailCount_; ++i) {
        const int idx = (trailHead_ + i) % MAX_TRAIL;
        float alpha = trail_[idx].life / 0.3f;
        if (alpha < 0.0f) alpha = 0.0f;
        if (alpha > 1.0f) alpha = 1.0f;

        Color trailColor = Fade(color, alpha * 0.5f);
        DrawCircleV(trail_[idx].pos, radius * (0.5f + static_cast<float>(i) * 0.05f), trailColor);
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

void PowerUp::Draw() const {
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

void Particle::Draw() const {
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

    const json perf = config_.value("performance", json::object());
    gridCols_ = perf.value("grid_cols", gridCols_);
    gridRows_ = perf.value("grid_rows", gridRows_);
    useSpatialGrid_ = perf.value("use_spatial_grid", useSpatialGrid_);
    showGridDebug_ = perf.value("show_grid", showGridDebug_);
    cellWidth_ = static_cast<float>(screenWidth_) / static_cast<float>(gridCols_);
    cellHeight_ = static_cast<float>(screenHeight_) / static_cast<float>(gridRows_);
    brickGrid_.assign(static_cast<size_t>(gridRows_),
                      std::vector<std::vector<size_t>>(static_cast<size_t>(gridCols_)));

    const std::string title =
        config_.value("screen", json::object()).value("title", std::string("Breakout Game"));
    InitWindow(screenWidth_, screenHeight_, title.c_str());
    InitAudioDevice();
    SetTargetFPS(60);
    SetExitKey(0);

    maxLevel_ = CountLevelFiles();
    RefreshCampaignSaveFlag();

    const int btnW = 220;
    const int btnH = 36;
    const int startX = screenWidth_ / 2 - btnW / 2;
    int menuY = 165;

    btnContinue_ = {(float)startX, (float)menuY, (float)btnW, (float)btnH};
    menuY += 44;

    btnRandomGame_ = {(float)startX, (float)menuY, (float)btnW, (float)btnH};
    menuY += 44;
    btnSelectGame_ = {(float)startX, (float)menuY, (float)btnW, (float)btnH};
    menuY += 44;
    btnSettings_ = {(float)startX, (float)menuY, (float)btnW, (float)btnH};
    menuY += 44;
    btnQuit_ = {(float)startX, (float)menuY, (float)btnW, (float)btnH};

    LoadLeaderboard();
    RefreshLevelPreviews();
    InitBackgroundDemoFromLevel(1 + (rand() % std::max(1, maxLevel_)));
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
        case GameState::LEVEL_SELECT:
            UpdateLevelSelect();
            break;
        case GameState::PLAYING:
            UpdatePlaying();
            break;
        case GameState::PAUSED:
            UpdatePaused();
            break;
        case GameState::LEVEL_CLEAR:
            UpdateLevelClear();
            break;
        case GameState::GAME_OVER:
            if (IsKeyPressed(KEY_ENTER)) {
                RefreshCampaignSaveFlag();
                state_ = GameState::MENU;
            }
            break;
        case GameState::LEADERBOARD:
            if (IsKeyPressed(KEY_ESCAPE)) state_ = GameState::MENU;
            break;
    }
}

void Game::UpdateMenu() {
    UpdateBackgroundDemo(GetFrameTime());

    if (hasPendingSave_ && IsButtonClicked(btnContinue_)) {
        ResumeCampaign();
    }
    if (IsButtonClicked(btnRandomGame_)) {
        StartRandomGame();
    }
    if (IsButtonClicked(btnSelectGame_)) {
        EnterLevelSelect();
    }
    if (IsButtonClicked(btnSettings_)) {
        state_ = GameState::LEADERBOARD;
    }
    if (IsButtonClicked(btnQuit_)) {
        exit(0);
    }
}

void Game::UpdatePaused() {
    if (IsKeyPressed(KEY_C)) {
        state_ = GameState::PLAYING;
    }
    if (IsKeyPressed(KEY_Q)) {
        if (!isRandomMode_) {
            SaveProgress();
        }
        RefreshCampaignSaveFlag();
        state_ = GameState::MENU;
    }
    if (IsKeyPressed(KEY_E)) {
        editingMode_ = !editingMode_;
        jsonStatusMessage_ = editingMode_ ? "编辑模式：左键添加 / 右键删除 / S保存" : "";
    }
    if (editingMode_) {
        UpdateEditor();
    }
}

void Game::ClampPaddle(Paddle& paddle) {
    if (paddle.position.x < 0.0f) paddle.position.x = 0.0f;
    if (paddle.position.x + paddle.width > screenWidth_)
        paddle.position.x = screenWidth_ - paddle.width;

    if (paddle.position.y < 0.0f) paddle.position.y = 0.0f;
    if (paddle.position.y + paddle.height > screenHeight_)
        paddle.position.y = screenHeight_ - paddle.height;
}

// 性能测量：GetTime 墙钟 + 每 60 帧 TraceLog（Total / Physics / Particles）
void Game::RecordUpdatePlayingLatency(double startSeconds) {
    lastUpdatePlayingMs_ = static_cast<float>((GetTime() - startSeconds) * 1000.0);
    estimatedDrawCalls_ = CountEstimatedDrawCalls();

    static int frameCounter = 0;
    if (frameCounter++ % 60 == 0) {
        TraceLog(LOG_INFO,
                 "Total: %.2fms | Physics: %.2fms | Particles: %.2fms | "
                 "Checks: %d/%d bricks | Grid: %s | DrawCalls~%d",
                 static_cast<double>(lastUpdatePlayingMs_),
                 static_cast<double>(lastBrickCollisionMs_),
                 static_cast<double>(lastParticleUpdateMs_),
                 lastCollisionChecks_,
                 activeBrickCount_,
                 useSpatialGrid_ ? "ON" : "OFF",
                 estimatedDrawCalls_);
    }
}

// 空间网格：关卡加载或编辑后重建；碰撞时只查球所在格及 3×3 邻域
void Game::RebuildCollisionGrid() {
    for (auto& row : brickGrid_) {
        for (auto& cell : row) {
            cell.clear();
        }
    }

    activeBrickCount_ = 0;
    for (size_t i = 0; i < bricks_.size(); ++i) {
        if (!bricks_[i].active) {
            continue;
        }
        ++activeBrickCount_;

        const Rectangle& br = bricks_[i].rect;
        const float cx = br.x + br.width * 0.5f;
        const float cy = br.y + br.height * 0.5f;

        int gx = static_cast<int>(cx / cellWidth_);
        int gy = static_cast<int>(cy / cellHeight_);
        if (gx < 0) gx = 0;
        if (gy < 0) gy = 0;
        if (gx >= gridCols_) gx = gridCols_ - 1;
        if (gy >= gridRows_) gy = gridRows_ - 1;

        brickGrid_[static_cast<size_t>(gy)][static_cast<size_t>(gx)].push_back(i);
    }
}

void Game::GetBallGridCell(int& gx, int& gy) const {
    gx = static_cast<int>(ball_.position.x / cellWidth_);
    gy = static_cast<int>(ball_.position.y / cellHeight_);
    if (gx < 0) gx = 0;
    if (gy < 0) gy = 0;
    if (gx >= gridCols_) gx = gridCols_ - 1;
    if (gy >= gridRows_) gy = gridRows_ - 1;
}

bool Game::ProcessBrickHit(size_t brickIndex) {
    Brick& brick = bricks_[brickIndex];
    if (!brick.active) {
        return false;
    }

    brick.active = false;
    --activeBrickCount_;
    ball_.velocity.y *= -1.0f;
    score_ += 1;

    PlayBrickHitSound(brick.color);

    EmitParticlesAtBrick(brick.rect, brick.color, 20);

    if ((rand() % 100) < 30) {
        PowerUpType type = static_cast<PowerUpType>(rand() % 3);
        PowerUp powerUp({ brick.rect.x + brick.rect.width / 2, brick.rect.y }, type);
        powerUp.duration = 5.0f;
        powerUps_.push_back(powerUp);
    }

    return true;
}

bool Game::CheckBallBrickCollisionsSpatial() {
    lastCollisionChecks_ = 0;
    lastCollisionCandidates_ = 0;

    const float ballR = ball_.radius;
    const float bx = ball_.position.x;
    const float by = ball_.position.y;

    int ballGx = 0;
    int ballGy = 0;
    GetBallGridCell(ballGx, ballGy);

    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            const int gx = ballGx + dx;
            const int gy = ballGy + dy;
            if (gx < 0 || gy < 0 || gx >= gridCols_ || gy >= gridRows_) {
                continue;
            }

            for (size_t idx : brickGrid_[static_cast<size_t>(gy)][static_cast<size_t>(gx)]) {
                ++lastCollisionCandidates_;
                Brick& brick = bricks_[idx];
                if (!brick.active) {
                    continue;
                }

                const Rectangle& br = brick.rect;
                if (bx + ballR < br.x || bx - ballR > br.x + br.width || by + ballR < br.y ||
                    by - ballR > br.y + br.height) {
                    continue;
                }

                ++lastCollisionChecks_;
                if (!CheckCollisionCircleRec(ball_.position, ballR, br)) {
                    continue;
                }

                return ProcessBrickHit(idx);
            }
        }
    }

    return false;
}

bool Game::CheckBallBrickCollisionsNaive() {
    lastCollisionChecks_ = 0;
    lastCollisionCandidates_ = activeBrickCount_;

    const float ballR = ball_.radius;
    const float bx = ball_.position.x;
    const float by = ball_.position.y;

    for (size_t i = 0; i < bricks_.size(); ++i) {
        if (!bricks_[i].active) {
            continue;
        }

        const Rectangle& br = bricks_[i].rect;
        if (bx + ballR < br.x || bx - ballR > br.x + br.width || by + ballR < br.y ||
            by - ballR > br.y + br.height) {
            continue;
        }

        ++lastCollisionChecks_;
        if (!CheckCollisionCircleRec(ball_.position, ballR, br)) {
            continue;
        }

        return ProcessBrickHit(i);
    }

    return false;
}

void Game::DrawCollisionGridDebug() const {
    for (int gx = 0; gx <= gridCols_; ++gx) {
        const float x = static_cast<float>(gx) * cellWidth_;
        DrawLineV({x, 0.0f}, {x, static_cast<float>(screenHeight_)}, Fade(SKYBLUE, 0.35f));
    }
    for (int gy = 0; gy <= gridRows_; ++gy) {
        const float y = static_cast<float>(gy) * cellHeight_;
        DrawLineV({0.0f, y}, {static_cast<float>(screenWidth_), y}, Fade(SKYBLUE, 0.35f));
    }

    int ballGx = 0;
    int ballGy = 0;
    GetBallGridCell(ballGx, ballGy);

    const Rectangle highlight = {
        static_cast<float>(ballGx) * cellWidth_,
        static_cast<float>(ballGy) * cellHeight_,
        cellWidth_,
        cellHeight_
    };
    DrawRectangleRec(highlight, Fade(YELLOW, 0.25f));
    DrawRectangleLinesEx(highlight, 2, ORANGE);

    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            const int gx = ballGx + dx;
            const int gy = ballGy + dy;
            if (gx < 0 || gy < 0 || gx >= gridCols_ || gy >= gridRows_) {
                continue;
            }
            const Rectangle neighbor = {
                static_cast<float>(gx) * cellWidth_,
                static_cast<float>(gy) * cellHeight_,
                cellWidth_,
                cellHeight_
            };
            DrawRectangleLinesEx(neighbor, 1, Fade(GREEN, 0.6f));
        }
    }
}

int Game::CountEstimatedDrawCalls() const {
    int count = 3; // ball + 2 paddles
    for (const auto& brick : bricks_) {
        if (brick.active) {
            ++count;
        }
    }
    for (const auto& p : powerUps_) {
        if (p.active) {
            ++count;
        }
    }
    count += activeParticleCount_;
    if (backgroundTexture_.id != 0) {
        ++count;
    }
    return count;
}

void Game::ClearParticlePool() {
    for (int i = 0; i < MAX_PARTICLES; ++i) {
        particleActive_[i] = false;
    }
    activeParticleCount_ = 0;
}

void Game::EmitParticlesAtBrick(const Rectangle& brickRect, Color color, int count) {
    int rw = static_cast<int>(brickRect.width);
    int rh = static_cast<int>(brickRect.height);
    if (rw < 1) {
        rw = 1;
    }
    if (rh < 1) {
        rh = 1;
    }
    int spawned = 0;
    for (int i = 0; i < MAX_PARTICLES && spawned < count; ++i) {
        if (particleActive_[i]) {
            continue;
        }
        const Vector2 pos = {
            brickRect.x + static_cast<float>(rand() % rw),
            brickRect.y + static_cast<float>(rand() % rh)
        };
        const Vector2 vel = {
            static_cast<float>((rand() % 200 - 100) / 10.0f),
            static_cast<float>((rand() % 200 - 100) / 10.0f)
        };
        particlePool_[i] = Particle(pos, vel, color, 0.5f);
        particleActive_[i] = true;
        ++activeParticleCount_;
        ++spawned;
    }
}

void Game::UpdateParticles(float dt) {
    for (int i = 0; i < MAX_PARTICLES; ++i) {
        if (!particleActive_[i]) {
            continue;
        }
        particlePool_[i].Update(dt);
        if (particlePool_[i].life <= 0.0f) {
            particleActive_[i] = false;
            --activeParticleCount_;
        }
    }
}

// UpdatePlaying：startFrame 为整帧墙钟；startCollision / startParticle 为分段计时（见下）。
// 粒子更新放在球/砖碰撞之前，以便球重生等待期内尾迹粒子仍每帧衰减（逻辑需要）。
void Game::UpdatePlaying() {
    const double startFrame = GetTime();
    lastParticleUpdateMs_ = 0.0f;
    lastBrickCollisionMs_ = 0.0f;
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
                ApplyLoadedLevel(data, pendingLoadLevel_);
            } catch (...) {
                isLoading_ = false;
                loadFailed_ = true;
                loadingMessage_ = "Load Failed!";
            }
        }
        RecordUpdatePlayingLatency(startFrame);
        return;
    }

    if (loadFailed_) {
        RecordUpdatePlayingLatency(startFrame);
        return;
    }

    if (IsKeyPressed(KEY_E)) {
        editingMode_ = !editingMode_;
        jsonStatusMessage_ = editingMode_ ? "编辑模式：左键添加 / 右键删除 / S保存" : "";
    }
    if (editingMode_) {
        UpdateEditor();
        if (IsKeyPressed(KEY_ESCAPE)) {
            state_ = GameState::PAUSED;
        }
        RecordUpdatePlayingLatency(startFrame);
        return;
    }

    if (IsKeyPressed(KEY_F5) && !isRandomMode_) {
        SaveProgress();
        jsonStatusMessage_ = "进度已保存";
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

    // 粒子更新（分段计时；对象池无 erase / 堆分配；须在球重生等待期之前执行）
    const double startParticle = GetTime();
    UpdateParticles(dt);
    lastParticleUpdateMs_ = static_cast<float>((GetTime() - startParticle) * 1000.0);

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
        RecordUpdatePlayingLatency(startFrame);
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

    // 球–砖碰撞（分段计时；网格法或朴素 O(N) 可切换对比）
    const double startCollision = GetTime();
    if (useSpatialGrid_) {
        (void)CheckBallBrickCollisionsSpatial();
    } else {
        (void)CheckBallBrickCollisionsNaive();
    }
    lastBrickCollisionMs_ = static_cast<float>((GetTime() - startCollision) * 1000.0);

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
            if (!isRandomMode_) {
                DeleteSave();
            }
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
        if (isRandomMode_) {
            StartLevelClearCelebration(true, 0, false);
        } else if (currentLevel_ < maxLevel_) {
            const int nextLevel = currentLevel_ + 1;
            SaveProgress(nextLevel);
            StartLevelClearCelebration(false, nextLevel, false);
        } else {
            DeleteSave();
            StartLevelClearCelebration(false, 0, true);
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

    // 性能演示快捷键（PPT 第4–9页）
    if (IsKeyPressed(KEY_G)) {
        showGridDebug_ = !showGridDebug_;
    }
    if (IsKeyPressed(KEY_B)) {
        useSpatialGrid_ = !useSpatialGrid_;
        TraceLog(LOG_INFO, "Collision mode: %s", useSpatialGrid_ ? "Spatial Grid" : "Naive O(N)");
    }
    if (IsKeyPressed(KEY_SPACE)) {
        EmitParticlesAtBrick(
            { ball_.position.x - ball_.radius, ball_.position.y - ball_.radius,
              ball_.radius * 2.0f, ball_.radius * 2.0f },
            ORANGE,
            100
        );
    }

    if (IsKeyPressed(KEY_ESCAPE)) state_ = GameState::PAUSED;

    RecordUpdatePlayingLatency(startFrame);
}

// ============================================================
// Reset
// ============================================================

void Game::SetupSessionDefaults() {
    lives_ = config_.value("game", json::object()).value("max_lives", 5);
    score_ = 0;

    const float speed = config_.value("ball", json::object()).value("speed_base", 4.0f);
    const float radius = config_.value("ball", json::object()).value("radius", 10.0f);
    ball_ = Ball({screenWidth_ / 2.0f, screenHeight_ / 2.0f}, {speed, -speed}, radius, RED);

    const float paddleWidth = config_.value("paddle", json::object()).value("width", 100.0f);
    const float paddleHeight = config_.value("paddle", json::object()).value("height", 10.0f);

    paddle1_.width = paddleWidth;
    paddle1_.height = paddleHeight;
    paddle1_.originalWidth = paddleWidth;
    paddle1_.position = {screenWidth_ * 0.25f - paddleWidth / 2.0f, (float)screenHeight_ - 50};
    paddle1_.color = BLUE;
    paddle1_.extendTimer = 0.0f;

    paddle2_.width = paddleWidth;
    paddle2_.height = paddleHeight;
    paddle2_.originalWidth = paddleWidth;
    paddle2_.position = {screenWidth_ * 0.75f - paddleWidth / 2.0f, (float)screenHeight_ - 50};
    paddle2_.color = GREEN;
    paddle2_.extendTimer = 0.0f;

    powerUps_.clear();
    ClearParticlePool();
    slowBallTimer_ = 0.0f;
    ballRespawnTimer_ = 0.0f;
    originalVelocities_.clear();
    editingMode_ = false;
    isLoading_ = false;
    loadReady_ = false;
    loadFailed_ = false;
    loadingMessage_.clear();
}

void Game::ResetGame() {
    SetupSessionDefaults();
    currentLevel_ = 1;
    bricks_.clear();
}

void Game::RefreshLevelPreviews() {
    levelPreviews_.clear();
    levelPreviews_.reserve(static_cast<size_t>(maxLevel_));
    for (int i = 1; i <= maxLevel_; ++i) {
        levelPreviews_.push_back(BuildLevelData(i));
    }
}

void Game::ShuffleBackgroundDemoLevels() {
    bgDemoLevelQueue_.clear();
    for (int i = 1; i <= maxLevel_; ++i) {
        bgDemoLevelQueue_.push_back(i);
    }
    std::random_device rd;
    std::mt19937 gen(rd());
    std::shuffle(bgDemoLevelQueue_.begin(), bgDemoLevelQueue_.end(), gen);
    bgDemoQueueIndex_ = 0;
}

void Game::AdvanceBackgroundDemoLevel() {
    if (bgDemoLevelQueue_.empty()) {
        return;
    }
    bgDemoQueueIndex_ = (bgDemoQueueIndex_ + 1) % static_cast<int>(bgDemoLevelQueue_.size());
    InitBackgroundDemoFromLevel(bgDemoLevelQueue_[static_cast<size_t>(bgDemoQueueIndex_)]);
}

void Game::InitBackgroundDemoFromLevel(int level) {
    if (levelPreviews_.empty()) {
        RefreshLevelPreviews();
    }
    const int idx = std::clamp(level, 1, maxLevel_) - 1;
    if (idx >= 0 && idx < static_cast<int>(levelPreviews_.size())) {
        InitBackgroundDemoFromLevelData(levelPreviews_[static_cast<size_t>(idx)]);
    }
}

void Game::InitBackgroundDemoFromLevelData(const LevelData& data) {
    bgDemoBricks_ = data.bricks;
    bgDemoBrickSnapshot_ = data.bricks;
    for (auto& brick : bgDemoBricks_) {
        brick.active = true;
    }

    const json ballCfg = config_.value("ball", json::object());
    const float speed = ballCfg.value("speed_base", 4.0f);
    const float radius = ballCfg.value("radius", 10.0f);
    bgDemoBall_ = Ball(
        {screenWidth_ / 2.0f, screenHeight_ * 0.55f},
        {speed * 1.15f, -speed * 1.1f},
        radius,
        Color{80, 220, 255, 255}
    );

    const json paddleCfg = config_.value("paddle", json::object());
    const float paddleWidth = paddleCfg.value("width", 100.0f);
    const float paddleHeight = paddleCfg.value("height", 10.0f);
    bgDemoPaddle1_ = Paddle(
        {screenWidth_ * 0.25f - paddleWidth / 2.0f, static_cast<float>(screenHeight_) - 50.0f},
        paddleWidth,
        paddleHeight,
        Color{70, 140, 255, 230}
    );
    bgDemoPaddle2_ = Paddle(
        {screenWidth_ * 0.75f - paddleWidth / 2.0f, static_cast<float>(screenHeight_) - 50.0f},
        paddleWidth,
        paddleHeight,
        Color{70, 255, 180, 230}
    );
    bgDemoPaddle1_.originalWidth = paddleWidth;
    bgDemoPaddle2_.originalWidth = paddleWidth;

    bgDemoPowerUps_.clear();
    bgDemoPowerUpSpawnTimer_ = 1.2f;
    bgDemoSimTime_ = 0.0f;
}

void Game::ResetBackgroundDemoBricks() {
    bgDemoBricks_ = bgDemoBrickSnapshot_;
    for (auto& brick : bgDemoBricks_) {
        brick.active = true;
    }
}

void Game::UpdateBackgroundDemo(float dt) {
    if (bgDemoBricks_.empty()) {
        return;
    }

    bgDemoSimTime_ += dt;
    bgDemoPaddle1_.Update(dt);
    bgDemoPaddle2_.Update(dt);

    const float targetX1 = bgDemoBall_.position.x - bgDemoPaddle1_.width * 0.5f;
    const float targetX2 = bgDemoBall_.position.x - bgDemoPaddle2_.width * 0.5f +
                           std::sin(bgDemoSimTime_ * 1.7f) * 40.0f;
    bgDemoPaddle1_.position.x += (targetX1 - bgDemoPaddle1_.position.x) * 4.5f * dt;
    bgDemoPaddle2_.position.x += (targetX2 - bgDemoPaddle2_.position.x) * 3.2f * dt;
    ClampPaddle(bgDemoPaddle1_);
    ClampPaddle(bgDemoPaddle2_);

    bgDemoBall_.Update(dt);

    if (bgDemoBall_.position.x - bgDemoBall_.radius <= 0.0f) {
        bgDemoBall_.position.x = bgDemoBall_.radius;
        bgDemoBall_.velocity.x *= -1.0f;
    } else if (bgDemoBall_.position.x + bgDemoBall_.radius >= screenWidth_) {
        bgDemoBall_.position.x = static_cast<float>(screenWidth_) - bgDemoBall_.radius;
        bgDemoBall_.velocity.x *= -1.0f;
    }
    if (bgDemoBall_.position.y - bgDemoBall_.radius <= 0.0f) {
        bgDemoBall_.position.y = bgDemoBall_.radius;
        bgDemoBall_.velocity.y *= -1.0f;
    }

    const Rectangle paddleRect1 = {
        bgDemoPaddle1_.position.x, bgDemoPaddle1_.position.y,
        bgDemoPaddle1_.width, bgDemoPaddle1_.height
    };
    const Rectangle paddleRect2 = {
        bgDemoPaddle2_.position.x, bgDemoPaddle2_.position.y,
        bgDemoPaddle2_.width, bgDemoPaddle2_.height
    };

    if (CheckCollisionCircleRec(bgDemoBall_.position, bgDemoBall_.radius, paddleRect1) &&
        bgDemoBall_.velocity.y > 0.0f) {
        bgDemoBall_.velocity.y *= -1.0f;
        const float hitPos = (bgDemoBall_.position.x - bgDemoPaddle1_.position.x) / bgDemoPaddle1_.width;
        bgDemoBall_.velocity.x = 8.0f * (hitPos - 0.5f);
    }
    if (CheckCollisionCircleRec(bgDemoBall_.position, bgDemoBall_.radius, paddleRect2) &&
        bgDemoBall_.velocity.y > 0.0f) {
        bgDemoBall_.velocity.y *= -1.0f;
        const float hitPos = (bgDemoBall_.position.x - bgDemoPaddle2_.position.x) / bgDemoPaddle2_.width;
        bgDemoBall_.velocity.x = 8.0f * (hitPos - 0.5f);
    }

    for (auto& brick : bgDemoBricks_) {
        if (!brick.active) {
            continue;
        }
        if (CheckCollisionCircleRec(bgDemoBall_.position, bgDemoBall_.radius, brick.rect)) {
            brick.active = false;
            bgDemoBall_.velocity.y *= -1.0f;
        }
    }

    if (bgDemoBall_.position.y - bgDemoBall_.radius > screenHeight_) {
        const json ballCfg = config_.value("ball", json::object());
        const float speed = ballCfg.value("speed_base", 4.0f);
        const float radius = ballCfg.value("radius", 10.0f);
        bgDemoBall_ = Ball(
            {screenWidth_ / 2.0f, screenHeight_ * 0.55f},
            {speed * (0.8f + static_cast<float>(rand() % 60) / 100.0f), -speed},
            radius,
            Color{80, 220, 255, 255}
        );
    }

    bool anyBrick = false;
    for (const auto& brick : bgDemoBricks_) {
        if (brick.active) {
            anyBrick = true;
            break;
        }
    }
    if (!anyBrick) {
        ResetBackgroundDemoBricks();
    }

    bgDemoPowerUpSpawnTimer_ -= dt;
    if (bgDemoPowerUpSpawnTimer_ <= 0.0f) {
        const float x = 40.0f + static_cast<float>(rand() % std::max(1, screenWidth_ - 80));
        const auto type = static_cast<PowerUpType>(rand() % 3);
        bgDemoPowerUps_.emplace_back(Vector2{x, 20.0f}, type);
        bgDemoPowerUpSpawnTimer_ = 2.0f + static_cast<float>(rand() % 25) / 10.0f;
    }

    for (auto& powerUp : bgDemoPowerUps_) {
        powerUp.Update(dt, static_cast<float>(screenHeight_));
    }
    bgDemoPowerUps_.erase(
        std::remove_if(bgDemoPowerUps_.begin(), bgDemoPowerUps_.end(),
                       [](const PowerUp& p) { return !p.active; }),
        bgDemoPowerUps_.end()
    );
}

namespace {

bool ColorsNear(Color a, Color b, int tolerance = 35) {
    return std::abs(static_cast<int>(a.r) - static_cast<int>(b.r)) <= tolerance &&
           std::abs(static_cast<int>(a.g) - static_cast<int>(b.g)) <= tolerance &&
           std::abs(static_cast<int>(a.b) - static_cast<int>(b.b)) <= tolerance;
}

}  // namespace

float Game::PitchFromBrickColor(Color brickColor) const {
    if (ColorsNear(brickColor, RED)) {
        return 0.78f;
    }
    if (ColorsNear(brickColor, ORANGE)) {
        return 0.88f;
    }
    if (ColorsNear(brickColor, YELLOW) || ColorsNear(brickColor, GOLD)) {
        return 1.0f;
    }
    if (ColorsNear(brickColor, GREEN)) {
        return 1.12f;
    }
    if (ColorsNear(brickColor, SKYBLUE)) {
        return 1.28f;
    }
    if (ColorsNear(brickColor, BLUE)) {
        return 1.42f;
    }
    if (ColorsNear(brickColor, PINK) || ColorsNear(brickColor, MAGENTA)) {
        return 1.58f;
    }
    if (ColorsNear(brickColor, PURPLE) || ColorsNear(brickColor, VIOLET)) {
        return 1.52f;
    }
    if (ColorsNear(brickColor, GRAY) || ColorsNear(brickColor, DARKGRAY)) {
        return 0.92f;
    }

    const float hueMix =
        (static_cast<float>(brickColor.r) * 0.6f + static_cast<float>(brickColor.g) * 0.3f +
         static_cast<float>(brickColor.b) * 0.1f) /
        255.0f;
    return 0.75f + hueMix * 0.85f;
}

void Game::PlayBrickHitSound(Color brickColor) const {
    if (!IsAudioDeviceReady() || hitSound_.frameCount <= 0) {
        return;
    }
    SetSoundPitch(hitSound_, PitchFromBrickColor(brickColor));
    PlaySound(hitSound_);
}

void Game::DrawTechBackground() const {
    ClearBackground(Color{10, 12, 20, 255});

    const Color gridMajor = {0, 200, 255, 22};
    const Color gridMinor = {0, 140, 200, 12};
    for (int x = 0; x <= screenWidth_; x += 20) {
        DrawLine(x, 0, x, screenHeight_, (x % 40 == 0) ? gridMajor : gridMinor);
    }
    for (int y = 0; y <= screenHeight_; y += 20) {
        DrawLine(0, y, screenWidth_, y, (y % 40 == 0) ? gridMajor : gridMinor);
    }

    DrawRectangleGradientV(0, 0, screenWidth_, screenHeight_,
                           Fade(Color{0, 80, 120, 255}, 0.08f),
                           Fade(BLACK, 0.55f));
}

void Game::DrawPlayingBackground() const {
    if (backgroundTexture_.id != 0) {
        DrawTexturePro(
            backgroundTexture_,
            Rectangle{0.0f, 0.0f, static_cast<float>(backgroundTexture_.width),
                      static_cast<float>(backgroundTexture_.height)},
            Rectangle{0.0f, 0.0f, static_cast<float>(screenWidth_), static_cast<float>(screenHeight_)},
            Vector2{0.0f, 0.0f},
            0.0f,
            Fade(WHITE, 0.1f)
        );
    }
}

void Game::DrawPlayingEntities() const {
    for (const auto& brick : bricks_) {
        if (!brick.active) {
            continue;
        }

        if (brickTexture_.id != 0) {
            DrawTexturePro(
                brickTexture_,
                Rectangle{0.0f, 0.0f, static_cast<float>(brickTexture_.width),
                          static_cast<float>(brickTexture_.height)},
                brick.rect,
                Vector2{0.0f, 0.0f},
                0.0f,
                brick.color
            );
        } else {
            DrawRectangleRec(brick.rect, brick.color);
        }
        DrawRectangleLinesEx(brick.rect, 1, Fade(Color{0, 220, 255, 255}, 0.35f));
    }

    for (const auto& powerUp : powerUps_) {
        powerUp.Draw();
    }

    ball_.Draw();
    paddle1_.Draw();
    paddle2_.Draw();

    for (int i = 0; i < MAX_PARTICLES; ++i) {
        if (particleActive_[i]) {
            particlePool_[i].Draw();
        }
    }
}

void Game::DrawBackgroundDemo() const {
    for (const auto& brick : bgDemoBricks_) {
        if (!brick.active) {
            continue;
        }
        Color c = brick.color;
        c.a = static_cast<unsigned char>(std::min(255, static_cast<int>(c.a) * 85 / 100));
        DrawRectangleRec(brick.rect, c);
        DrawRectangleLinesEx(brick.rect, 1, Fade(Color{0, 220, 255, 255}, 0.25f));
    }

    for (const auto& powerUp : bgDemoPowerUps_) {
        powerUp.Draw();
    }

    bgDemoPaddle1_.Draw();
    bgDemoPaddle2_.Draw();
    bgDemoBall_.Draw();
}

void Game::SpawnFireworkBurst(Vector2 center) {
    const Color palette[] = {
        Color{255, 80, 120, 255}, Color{255, 180, 60, 255}, Color{255, 240, 120, 255},
        Color{80, 220, 255, 255}, Color{120, 255, 160, 255}, Color{200, 120, 255, 255},
        Color{255, 120, 200, 255}
    };

    for (int i = 0; i < 48; ++i) {
        const float angle = static_cast<float>(rand() % 360) * DEG2RAD;
        const float speed = 90.0f + static_cast<float>(rand() % 160);
        FireworkSpark spark;
        spark.pos = center;
        spark.vel = {std::cos(angle) * speed, std::sin(angle) * speed};
        spark.color = palette[rand() % 7];
        spark.maxLife = 0.7f + static_cast<float>(rand() % 30) / 100.0f;
        spark.life = spark.maxLife;
        fireworks_.push_back(spark);
    }
}

void Game::UpdateFireworks(float dt) {
    for (auto& spark : fireworks_) {
        spark.pos.x += spark.vel.x * dt;
        spark.pos.y += spark.vel.y * dt;
        spark.vel.y += 220.0f * dt;
        spark.vel.x *= 0.98f;
        spark.life -= dt;
    }

    fireworks_.erase(
        std::remove_if(fireworks_.begin(), fireworks_.end(),
                       [](const FireworkSpark& s) { return s.life <= 0.0f; }),
        fireworks_.end()
    );
}

void Game::DrawFireworks() const {
    for (const auto& spark : fireworks_) {
        const float t = (spark.maxLife > 0.0f) ? (spark.life / spark.maxLife) : 0.0f;
        const float alpha = std::clamp(t, 0.0f, 1.0f);
        const float radius = 2.0f + (1.0f - alpha) * 3.0f;
        DrawCircleV(spark.pos, radius, Fade(spark.color, alpha));
    }
}

void Game::StartLevelClearCelebration(bool randomMode, int nextLevel, bool isFinalVictory) {
    levelClearRandomMode_ = randomMode;
    levelClearPendingLevel_ = nextLevel;
    levelClearIsFinal_ = isFinalVictory;
    levelClearPhase_ = LevelClearPhase::FIREWORKS;
    levelClearTimer_ = 0.0f;
    fireworksBurstTimer_ = 0.0f;
    fireworks_.clear();
    state_ = GameState::LEVEL_CLEAR;
    SpawnFireworkBurst({screenWidth_ / 2.0f, screenHeight_ * 0.32f});
}

void Game::InitLevelClearDialogButtons() {
    const int btnW = 200;
    const int btnH = 44;
    const int gap = 24;
    const int totalW = btnW * 2 + gap;
    const int startX = screenWidth_ / 2 - totalW / 2;
    const int y = screenHeight_ / 2 + 40;

    if (levelClearIsFinal_) {
        btnLevelClearNext_ = {0, 0, 0, 0};
        btnLevelClearQuit_ = {
            static_cast<float>(screenWidth_ / 2 - btnW / 2),
            static_cast<float>(y),
            static_cast<float>(btnW),
            static_cast<float>(btnH)
        };
    } else {
        btnLevelClearNext_ = {
            static_cast<float>(startX),
            static_cast<float>(y),
            static_cast<float>(btnW),
            static_cast<float>(btnH)
        };
        btnLevelClearQuit_ = {
            static_cast<float>(startX + btnW + gap),
            static_cast<float>(y),
            static_cast<float>(btnW),
            static_cast<float>(btnH)
        };
    }
}

void Game::UpdateLevelClear() {
    const float dt = GetFrameTime();
    levelClearTimer_ += dt;
    UpdateFireworks(dt);

    fireworksBurstTimer_ += dt;
    if (levelClearPhase_ == LevelClearPhase::FIREWORKS && fireworksBurstTimer_ >= 0.32f) {
        fireworksBurstTimer_ = 0.0f;
        SpawnFireworkBurst({
            static_cast<float>(80 + rand() % std::max(1, screenWidth_ - 160)),
            static_cast<float>(60 + rand() % std::max(1, screenHeight_ / 2))
        });
    }

    if (levelClearPhase_ == LevelClearPhase::FIREWORKS) {
        static constexpr float kFireworksDuration = 2.5f;
        if (levelClearTimer_ >= kFireworksDuration) {
            if (levelClearRandomMode_) {
                fireworks_.clear();
                state_ = GameState::PLAYING;
                StartRandomLevelLoad();
            } else {
                levelClearPhase_ = LevelClearPhase::DIALOG;
                levelClearTimer_ = 0.0f;
                InitLevelClearDialogButtons();
            }
        }
        return;
    }

    if (!levelClearIsFinal_ && btnLevelClearNext_.width > 0.0f && IsButtonClicked(btnLevelClearNext_)) {
        fireworks_.clear();
        state_ = GameState::PLAYING;
        StartLevelLoad(levelClearPendingLevel_);
        return;
    }

    if (IsButtonClicked(btnLevelClearQuit_)) {
        fireworks_.clear();
        if (levelClearIsFinal_) {
            SaveScore();
            state_ = GameState::GAME_OVER;
        } else {
            RefreshCampaignSaveFlag();
            state_ = GameState::MENU;
            InitBackgroundDemoFromLevel(1 + (rand() % std::max(1, maxLevel_)));
        }
    }
}

void Game::DrawGameplaySnapshot() const {
    DrawPlayingEntities();
}

void Game::DrawLevelClearOverlay() const {
    DrawRectangle(0, 0, screenWidth_, screenHeight_, Fade(BLACK, 0.35f));
    DrawFireworks();

    if (levelClearPhase_ == LevelClearPhase::FIREWORKS) {
        const char* msg = levelClearRandomMode_ ? "Level Clear!" : "Stage Complete!";
        const int tw = MeasureText(msg, 36);
        DrawText(msg, screenWidth_ / 2 - tw / 2, 80, 36, Color{120, 240, 255, 255});
        return;
    }

    const int panelW = 420;
    const int panelH = levelClearIsFinal_ ? 200 : 220;
    const int px = screenWidth_ / 2 - panelW / 2;
    const int py = screenHeight_ / 2 - panelH / 2;

    DrawRectangle(px, py, panelW, panelH, Fade(Color{18, 24, 38, 255}, 0.92f));
    DrawRectangleLines(px, py, panelW, panelH, Color{0, 200, 255, 200});

    const char* title = levelClearIsFinal_ ? "Campaign Complete!" : "Level Complete!";
    const int titleW = MeasureText(title, 28);
    DrawText(title, screenWidth_ / 2 - titleW / 2, py + 24, 28, Color{120, 240, 255, 255});

    if (!levelClearIsFinal_) {
        const char* sub = TextFormat("Ready for Level %d?", levelClearPendingLevel_);
        const int subW = MeasureText(sub, 18);
        DrawText(sub, screenWidth_ / 2 - subW / 2, py + 62, 18, LIGHTGRAY);
    } else {
        const char* sub = TextFormat("Final Score: %d", score_);
        const int subW = MeasureText(sub, 18);
        DrawText(sub, screenWidth_ / 2 - subW / 2, py + 62, 18, LIGHTGRAY);
    }

    const Vector2 mouse = GetMousePosition();

    if (!levelClearIsFinal_ && btnLevelClearNext_.width > 0.0f) {
        const bool hoverNext = CheckCollisionPointRec(mouse, btnLevelClearNext_);
        DrawRectangleRec(btnLevelClearNext_,
                         hoverNext ? Color{60, 180, 255, 255} : Color{40, 120, 200, 255});
        DrawRectangleLinesEx(btnLevelClearNext_, 2, Color{0, 220, 255, 255});
        const char* nextLabel = "Next Level";
        const int nw = MeasureText(nextLabel, 20);
        DrawText(nextLabel,
                 static_cast<int>(btnLevelClearNext_.x + (btnLevelClearNext_.width - nw) / 2.0f),
                 static_cast<int>(btnLevelClearNext_.y + 12),
                 20,
                 WHITE);
    }

    const bool hoverQuit = CheckCollisionPointRec(mouse, btnLevelClearQuit_);
    DrawRectangleRec(btnLevelClearQuit_,
                     hoverQuit ? Color{255, 100, 100, 255} : Color{180, 70, 70, 255});
    DrawRectangleLinesEx(btnLevelClearQuit_, 2, Color{255, 160, 160, 255});
    const char* quitLabel = levelClearIsFinal_ ? "Quit to Results" : "Quit";
    const int qw = MeasureText(quitLabel, 20);
    DrawText(quitLabel,
             static_cast<int>(btnLevelClearQuit_.x + (btnLevelClearQuit_.width - qw) / 2.0f),
             static_cast<int>(btnLevelClearQuit_.y + 12),
             20,
             WHITE);
}

bool Game::HasCampaignSave() const {
    if (!SaveFileExists(SAVE_PATH)) {
        return false;
    }

    try {
        std::ifstream file(SAVE_PATH);
        json save;
        file >> save;
        return save.value("campaign", false);
    } catch (...) {
        return false;
    }
}

void Game::RefreshCampaignSaveFlag() {
    hasPendingSave_ = HasCampaignSave();
}

void Game::EnterLevelSelect() {
    isRandomMode_ = false;
    RefreshLevelPreviews();
    ShuffleBackgroundDemoLevels();
    bgDemoCycleTimer_ = 0.0f;
    if (!bgDemoLevelQueue_.empty()) {
        InitBackgroundDemoFromLevel(bgDemoLevelQueue_[0]);
    }
    state_ = GameState::LEVEL_SELECT;
    SetMouseCursor(MOUSE_CURSOR_DEFAULT);
}

void Game::ResumeCampaign() {
    if (!HasCampaignSave()) {
        jsonStatusMessage_ = "请先通过 Select Game 开始战役";
        return;
    }

    if (!LoadGame()) {
        jsonStatusMessage_ = "战役存档损坏";
        RefreshCampaignSaveFlag();
        return;
    }

    isRandomMode_ = false;
    editingMode_ = false;

    const float speed = config_.value("ball", json::object()).value("speed_base", 4.0f);
    const float radius = config_.value("ball", json::object()).value("radius", 10.0f);
    ball_ = Ball({screenWidth_ / 2.0f, screenHeight_ / 2.0f}, {speed, -speed}, radius, RED);

    const float paddleWidth = config_.value("paddle", json::object()).value("width", 100.0f);
    const float paddleHeight = config_.value("paddle", json::object()).value("height", 10.0f);
    paddle1_.height = paddleHeight;
    paddle2_.height = paddleHeight;
    paddle1_.position = {screenWidth_ * 0.25f - paddleWidth / 2.0f, (float)screenHeight_ - 50};
    paddle2_.position = {screenWidth_ * 0.75f - paddleWidth / 2.0f, (float)screenHeight_ - 50};

    powerUps_.clear();
    ClearParticlePool();
    ballRespawnTimer_ = 0.0f;
    originalVelocities_.clear();

    bricks_.clear();
    isLoading_ = false;
    loadFailed_ = false;

    state_ = GameState::PLAYING;
    jsonStatusMessage_ = TextFormat("继续战役：第 %d 关", currentLevel_);
    StartLevelLoad(currentLevel_);
}

void Game::StartCampaignLevel(int level) {
    isRandomMode_ = false;
    editingMode_ = false;
    SetupSessionDefaults();

    currentLevel_ = level;
    state_ = GameState::PLAYING;
    StartLevelLoad(level);
}

void Game::StartRandomGame() {
    isRandomMode_ = true;
    editingMode_ = false;
    SetupSessionDefaults();
    currentLevel_ = 0;
    state_ = GameState::PLAYING;
    StartRandomLevelLoad();
}

void Game::UpdateLevelSelect() {
    const float dt = GetFrameTime();
    UpdateBackgroundDemo(dt);

    bgDemoCycleTimer_ += dt;
    if (bgDemoCycleTimer_ >= kBgDemoCycleSeconds) {
        bgDemoCycleTimer_ = 0.0f;
        AdvanceBackgroundDemoLevel();
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        state_ = GameState::MENU;
        InitBackgroundDemoFromLevel(1 + (rand() % std::max(1, maxLevel_)));
        SetMouseCursor(MOUSE_CURSOR_DEFAULT);
        return;
    }

    const int cardW = 120;
    const int cardH = 88;
    const int labelH = 32;
    const int gap = 16;
    const int totalW = maxLevel_ * cardW + (maxLevel_ - 1) * gap;
    const float startX = (screenWidth_ - totalW) / 2.0f;
    const float previewY = 150.0f;
    const float labelY = previewY + cardH + 8.0f;

    bool anyHover = false;

    for (int i = 0; i < maxLevel_; ++i) {
        const float x = startX + static_cast<float>(i * (cardW + gap));
        const Rectangle previewRect = {x, previewY, static_cast<float>(cardW), static_cast<float>(cardH)};
        const Rectangle labelRect = {x, labelY, static_cast<float>(cardW), static_cast<float>(labelH)};
        const bool hoverPreview = CheckCollisionPointRec(GetMousePosition(), previewRect);
        const bool hoverLabel = CheckCollisionPointRec(GetMousePosition(), labelRect);
        const bool hover = hoverPreview || hoverLabel;

        if (hover) {
            anyHover = true;
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                StartCampaignLevel(i + 1);
                return;
            }
        }
    }

    SetMouseCursor(anyHover ? MOUSE_CURSOR_POINTING_HAND : MOUSE_CURSOR_DEFAULT);
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
    const json fallback = {
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
    config_ = LoadJSONWithFallback("config.json", fallback);
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
    DrawFPS(10, 10);
    DrawText(TextFormat("Update: %.2f ms", lastUpdatePlayingMs_), 10, 35, 18, YELLOW);
    DrawText(TextFormat("Particles: %d / %d", activeParticleCount_, MAX_PARTICLES), 10, 58, 18, GREEN);
    DrawText(TextFormat("Part Δt: %.3f ms", lastParticleUpdateMs_), 10, 81, 16, DARKGRAY);
    DrawText(TextFormat("Physics: %.3f ms", lastBrickCollisionMs_), 10, 100, 16, DARKGRAY);

    // 简易性能条：绿 = Physics（砖碰撞），蓝 = Particles；满条 = 4 ms
    {
        const int barX = 10;
        const int barY0 = 118;
        const int barMaxW = 180;
        const int barH = 10;
        const float capMs = 4.0f;
        const float physT = std::min(1.0f, lastBrickCollisionMs_ / capMs);
        const float partT = std::min(1.0f, lastParticleUpdateMs_ / capMs);
        const int physW = static_cast<int>(physT * static_cast<float>(barMaxW));
        const int partW = static_cast<int>(partT * static_cast<float>(barMaxW));

        DrawText("Physics", barX + barMaxW + 8, barY0 - 1, 14, DARKGREEN);
        DrawRectangle(barX, barY0, barMaxW, barH, Fade(LIGHTGRAY, 0.45f));
        DrawRectangle(barX, barY0, physW, barH, Fade(GREEN, 0.9f));

        DrawText("Particles", barX + barMaxW + 8, barY0 + barH + 5, 14, DARKBLUE);
        DrawRectangle(barX, barY0 + barH + 4, barMaxW, barH, Fade(LIGHTGRAY, 0.45f));
        DrawRectangle(barX, barY0 + barH + 4, partW, barH, Fade(BLUE, 0.9f));
    }

    DrawText(TextFormat("Score: %d", score_), 10, 158, 20, Color{120, 240, 255, 255});
    DrawText(TextFormat("Lives: %d", lives_), 10, 188, 20, Color{255, 200, 120, 255});
    DrawText(TextFormat("Level: %d/%d", currentLevel_, maxLevel_), 10, 218, 20, LIGHTGRAY);

    if (paddle1_.extendTimer > 0.0f) {
        DrawText(TextFormat("P1 Extend: %.1f", paddle1_.extendTimer), 10, 248, 20, BLUE);
    }
    if (paddle2_.extendTimer > 0.0f) {
        DrawText(TextFormat("P2 Extend: %.1f", paddle2_.extendTimer), 10, 278, 20, GREEN);
    }
    if (slowBallTimer_ > 0.0f) {
        DrawText(TextFormat("Slow Ball: %.1f", slowBallTimer_), 10, 308, 20, YELLOW);
    }

    DrawText(TextFormat("Collision: %s", useSpatialGrid_ ? "Grid" : "Naive"), 10, 338, 16, DARKGREEN);
    DrawText(TextFormat("Checks: %d (cand %d / %d bricks)", lastCollisionChecks_,
                        lastCollisionCandidates_, activeBrickCount_),
             10, 358, 14, DARKGRAY);
    DrawText(TextFormat("DrawCalls~: %d", estimatedDrawCalls_), 10, 378, 14, DARKGRAY);
    DrawText("G:grid  B:toggle  Space:particles  F5:save", 10, 398, 12, GRAY);
    if (!jsonStatusMessage_.empty()) {
        DrawText(jsonStatusMessage_.c_str(), 10, 418, 14, MAROON);
    }
}

void Game::DrawLevelPreview(const LevelData& data, Rectangle bounds, float scale) const {
    if (data.bricks.empty()) {
        DrawRectangleRec(bounds, Fade(LIGHTGRAY, 0.5f));
        return;
    }

    float minX = data.bricks[0].rect.x;
    float minY = data.bricks[0].rect.y;
    float maxX = minX + data.bricks[0].rect.width;
    float maxY = minY + data.bricks[0].rect.height;

    for (const auto& brick : data.bricks) {
        minX = std::min(minX, brick.rect.x);
        minY = std::min(minY, brick.rect.y);
        maxX = std::max(maxX, brick.rect.x + brick.rect.width);
        maxY = std::max(maxY, brick.rect.y + brick.rect.height);
    }

    const float layoutW = std::max(1.0f, maxX - minX);
    const float layoutH = std::max(1.0f, maxY - minY);
    const float pad = 6.0f;
    const float fitScale = std::min((bounds.width - pad * 2.0f) / layoutW,
                                    (bounds.height - pad * 2.0f) / layoutH) * scale;

    const float drawOffX = bounds.x + (bounds.width - layoutW * fitScale) * 0.5f;
    const float drawOffY = bounds.y + (bounds.height - layoutH * fitScale) * 0.5f;

    DrawRectangleRec(bounds, Fade(RAYWHITE, 0.92f));
    DrawRectangleLinesEx(bounds, 2, Fade(DARKGRAY, 0.6f));

    for (const auto& brick : data.bricks) {
        const Rectangle r = {
            drawOffX + (brick.rect.x - minX) * fitScale,
            drawOffY + (brick.rect.y - minY) * fitScale,
            brick.rect.width * fitScale,
            brick.rect.height * fitScale
        };
        DrawRectangleRec(r, brick.color);
    }
}

void Game::DrawLevelSelect() {
    DrawBackgroundDemo();
    DrawRectangle(0, 0, screenWidth_, screenHeight_, Fade(BLACK, 0.58f));

    DrawText("SELECT LEVEL", screenWidth_ / 2 - 90, 50, 32, Color{120, 240, 255, 255});

    DrawText("Choose a level to start campaign", screenWidth_ / 2 - 130, 88, 16, LIGHTGRAY);

    const int cardW = 120;
    const int cardH = 88;
    const int labelH = 32;
    const int gap = 16;
    const int totalW = maxLevel_ * cardW + (maxLevel_ - 1) * gap;
    const float startX = (screenWidth_ - totalW) / 2.0f;
    const float previewY = 150.0f;
    const float labelY = previewY + cardH + 8.0f;
    const Vector2 mouse = GetMousePosition();

    for (int i = 0; i < maxLevel_; ++i) {
        const int levelNum = i + 1;
        const float x = startX + static_cast<float>(i * (cardW + gap));
        Rectangle previewRect = {x, previewY, static_cast<float>(cardW), static_cast<float>(cardH)};
        Rectangle labelRect = {x, labelY, static_cast<float>(cardW), static_cast<float>(labelH)};

        const bool hover = CheckCollisionPointRec(mouse, previewRect) ||
                           CheckCollisionPointRec(mouse, labelRect);
        const float scale = hover ? 1.08f : 1.0f;
        const float growW = previewRect.width * (scale - 1.0f);
        const float growH = (previewRect.height + labelRect.height + 8.0f) * (scale - 1.0f);

        previewRect.x -= growW * 0.5f;
        previewRect.y -= growH * 0.5f;
        previewRect.width += growW;
        previewRect.height += growH;
        labelRect.x -= growW * 0.5f;
        labelRect.y -= growH * 0.5f;
        labelRect.width += growW;
        labelRect.height += growH * 0.35f;

        const LevelData& preview =
            (i < static_cast<int>(levelPreviews_.size())) ? levelPreviews_[static_cast<size_t>(i)] : LevelData{};
        DrawLevelPreview(preview, previewRect, 1.0f);

        const Color labelBg = hover ? Color{60, 160, 255, 240} : Color{30, 40, 55, 220};
        DrawRectangleRec(labelRect, labelBg);
        DrawRectangleLinesEx(labelRect, 2, Color{0, 200, 255, 180});

        const char* label = TextFormat("Level %d", levelNum);
        const int tw = MeasureText(label, 18);
        DrawText(label,
                 static_cast<int>(labelRect.x + (labelRect.width - tw) / 2.0f),
                 static_cast<int>(labelRect.y + 6.0f),
                 18,
                 WHITE);
    }

    DrawText("ESC: Back to Menu", screenWidth_ / 2 - 70, screenHeight_ - 40, 16, GRAY);
}

void Game::DrawMenu() {
    DrawBackgroundDemo();
    DrawRectangle(0, 0, screenWidth_, screenHeight_, Fade(BLACK, 0.62f));
    DrawText("BREAKOUT", screenWidth_ / 2 - 80, 100, 40, Color{120, 240, 255, 255});

    struct MenuBtn {
        Rectangle* rect;
        const char* label;
        int xOffset;
        Color hoverColor;
        Color normalColor;
    };

    MenuBtn items[5];
    int count = 0;

    {
        const bool canContinue = hasPendingSave_;
        items[count++] = {
            &btnContinue_,
            "CONTINUE",
            50,
            canContinue ? Color{100, 200, 255, 255} : Color{160, 160, 160, 255},
            canContinue ? Color{200, 230, 255, 255} : Color{210, 210, 210, 255}
        };
    }
    items[count++] = {&btnRandomGame_, "RANDOM GAME", 35, {255, 140, 80, 255}, {255, 220, 180, 255}};
    items[count++] = {&btnSelectGame_, "SELECT GAME", 35, {255, 100, 100, 255}, LIGHTGRAY};
    items[count++] = {&btnSettings_, "SETTINGS", 45, {100, 255, 100, 255}, LIGHTGRAY};
    items[count++] = {&btnQuit_, "QUIT", 70, {100, 100, 255, 255}, LIGHTGRAY};

    for (int i = 0; i < count; ++i) {
        const MenuBtn& item = items[i];
        const bool isContinueBtn = item.rect == &btnContinue_;
        const bool enabled = !isContinueBtn || hasPendingSave_;
        const bool hover = enabled && CheckCollisionPointRec(GetMousePosition(), *item.rect);
        const float scale = hover ? 1.1f : 1.0f;
        const Color color = hover ? item.hoverColor : item.normalColor;

        const Rectangle scaledRect = {
            item.rect->x - item.rect->width * (scale - 1.0f) / 2.0f,
            item.rect->y - item.rect->height * (scale - 1.0f) / 2.0f,
            item.rect->width * scale,
            item.rect->height * scale
        };

        DrawRectangleRec(scaledRect, color);
        DrawText(item.label,
                 static_cast<int>(scaledRect.x + item.xOffset * scale),
                 static_cast<int>(scaledRect.y + 10.0f * scale),
                 static_cast<int>(20.0f * scale),
                 WHITE);
    }
}

void Game::Draw() {
    BeginDrawing();

    if (state_ == GameState::MENU || state_ == GameState::LEVEL_SELECT ||
        state_ == GameState::PLAYING || state_ == GameState::PAUSED ||
        state_ == GameState::LEVEL_CLEAR) {
        DrawTechBackground();
    } else {
        ClearBackground(RAYWHITE);
    }

    switch (state_) {
        case GameState::MENU:
            DrawMenu();
            break;

        case GameState::LEVEL_SELECT:
            DrawLevelSelect();
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
                    DrawPlayingBackground();

                    if (showGridDebug_) {
                        DrawCollisionGridDebug();
                    }

                    DrawPlayingEntities();
                    DrawUI();
                    DrawEditorOverlay();

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
            DrawPlayingBackground();
            DrawPlayingEntities();
            DrawUI();
            DrawEditorOverlay();
            DrawRectangle(0, 0, screenWidth_, screenHeight_, {0, 0, 0, 150});
            DrawRectangle(screenWidth_ / 2 - 100, screenHeight_ / 2 - 50, 200, 100, LIGHTGRAY);
            DrawText("PAUSED", screenWidth_ / 2 - 40, screenHeight_ / 2 - 40, 20, BLACK);
            DrawText("Continue (C) / Quit (Q)", screenWidth_ / 2 - 100, screenHeight_ / 2, 15, BLACK);
            break;

        case GameState::LEVEL_CLEAR: {
            DrawPlayingBackground();
            DrawGameplaySnapshot();
            DrawLevelClearOverlay();
            break;
        }

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

Color Game::ColorFromPatternChar(char ch) const {
    switch (ch) {
        case 'R': case 'r': return RED;
        case 'G': case 'g': return GREEN;
        case 'B': case 'b': return BLUE;
        case 'Y': case 'y': return YELLOW;
        case 'C': case 'c': return SKYBLUE;
        case 'O': case 'o': return ORANGE;
        case 'P': case 'p': return PINK;
        default: return GRAY;
    }
}

Color Game::ColorFromLayoutCode(int code, const json& colorMap) const {
    if (code == 0) {
        return BLANK;
    }

    const std::string key = std::to_string(code);
    if (colorMap.contains(key)) {
        const std::string name = colorMap[key].get<std::string>();
        if (name == "red") return RED;
        if (name == "yellow" || name == "gold") return GOLD;
        if (name == "green") return GREEN;
        if (name == "blue") return BLUE;
        if (name == "orange") return ORANGE;
        if (name == "pink") return PINK;
        if (name == "cyan" || name == "skyblue") return SKYBLUE;
    }

    if (code == 1) return RED;
    if (code == 2) return GOLD;
    if (code == 3) return MAROON;
    return GRAY;
}

json Game::GetDefaultLevelJson(int level) const {
    json fallback = {
        {"background", "assets/bg_level1.png"},
        {"brick_texture", "assets/brick_level1.png"},
        {"hit_sound", "assets/hit1.wav"},
        {"brick_width", 75},
        {"brick_height", 20},
        {"spacing", 5},
        {"offset_x", 60},
        {"offset_y", 60},
        {"pattern", json::array({"RRRRRRRR", "GGGGGGGG"})}
    };

    if (level == 2) {
        fallback["background"] = "assets/bg_level2.png";
        fallback["brick_texture"] = "assets/brick_level2.png";
        fallback["hit_sound"] = "assets/hit2.wav";
        fallback["pattern"] = json::array({"RRRRRRRRRR", "GGGGGGGGGG", "BBBBBBBBBB"});
    } else if (level == 3) {
        fallback["background"] = "assets/bg_level3.png";
        fallback["brick_texture"] = "assets/brick_level3.png";
        fallback["hit_sound"] = "assets/hit3.wav";
        fallback["pattern"] = json::array({"R...........R", "RG.........GR", "RGB.......BGR"});
    } else if (level == 4) {
        fallback["background"] = "assets/bg_level2.png";
        fallback["brick_texture"] = "assets/brick_level2.png";
        fallback["hit_sound"] = "assets/hit2.wav";
        fallback["pattern"] = json::array({"R.R.R.R.R", ".RRRRRRR.", "RRRRRRRRR", ".RRRRRRR.", "R.R.R.R.R"});
    } else if (level == 5) {
        fallback["background"] = "assets/bg_level3.png";
        fallback["brick_texture"] = "assets/brick_level3.png";
        fallback["hit_sound"] = "assets/hit3.wav";
        fallback["pattern"] = json::array({"RRRRRRRRRR", "RRRRRRRRRR", "..........", "RRRRRRRRRR", "RRRRRRRRRR", "..........", "RRRRRRRRRR"});
    }

    return fallback;
}

// 数据驱动关卡：优先 pattern 字符串，否则 bricks.layout + color_map
LevelData Game::ParseLevelJson(const json& levelJson, int level) const {
    LevelData data;
    data.backgroundTexturePath = levelJson.value("background", "assets/bg_level1.png");
    data.brickTexturePath = levelJson.value("brick_texture", "assets/brick_level1.png");
    data.hitSoundPath = levelJson.value("hit_sound", "assets/hit1.wav");

    const json bricksCfg = levelJson.value("bricks", json::object());
    const float bWidth = levelJson.value("brick_width", bricksCfg.value("width", 75.0f));
    const float bHeight = levelJson.value("brick_height", bricksCfg.value("height", 20.0f));
    const float spacing = levelJson.value("spacing", bricksCfg.value("spacing", 5.0f));
    const float offsetX = levelJson.value("offset_x", bricksCfg.value("offset_x", 60.0f));
    const float offsetY = levelJson.value("offset_y", bricksCfg.value("offset_y", 60.0f));

    data.bricks.clear();

    if (levelJson.contains("pattern") && levelJson["pattern"].is_array()) {
        const auto& pattern = levelJson["pattern"];
        for (size_t r = 0; r < pattern.size(); ++r) {
            const std::string row = pattern[r].get<std::string>();
            for (size_t c = 0; c < row.size(); ++c) {
                const char ch = row[c];
                if (ch == '.' || ch == '0' || ch == ' ') {
                    continue;
                }
                data.bricks.emplace_back(
                    Vector2{offsetX + static_cast<float>(c) * (bWidth + spacing),
                            offsetY + static_cast<float>(r) * (bHeight + spacing)},
                    bWidth,
                    bHeight,
                    ColorFromPatternChar(ch)
                );
            }
        }
        return data;
    }

    if (bricksCfg.contains("layout") && bricksCfg["layout"].is_array()) {
        const auto& layout = bricksCfg["layout"];
        const int rows = bricksCfg.value("rows", static_cast<int>(layout.size()));
        const int cols = bricksCfg.value("cols", layout.empty() ? 0 : static_cast<int>(layout[0].size()));
        const json colorMap = bricksCfg.value("color_map", json::object());

        for (int i = 0; i < rows; ++i) {
            if (i >= static_cast<int>(layout.size())) {
                break;
            }
            for (int j = 0; j < cols; ++j) {
                if (j >= static_cast<int>(layout[i].size())) {
                    break;
                }
                const int code = layout[i][j].get<int>();
                if (code == 0) {
                    continue;
                }
                data.bricks.emplace_back(
                    Vector2{offsetX + static_cast<float>(j) * (bWidth + spacing),
                            offsetY + static_cast<float>(i) * (bHeight + spacing)},
                    bWidth,
                    bHeight,
                    ColorFromLayoutCode(code, colorMap)
                );
            }
        }
        return data;
    }

    TraceLog(LOG_WARNING, "关卡 %d JSON 缺少 pattern/layout，使用空布局", level);
    return data;
}

int Game::CountLevelFiles() const {
    int count = 0;
    for (int i = 1; i <= 99; ++i) {
        const std::string path = "levels/level" + std::to_string(i) + ".json";
        if (!SaveFileExists(path)) {
            break;
        }
        count = i;
    }
    return count > 0 ? count : 3;
}

// 序列化游戏状态到 save.json（version 字段用于存档迁移）
void Game::SaveProgress(int levelOverride) {
    if (isRandomMode_) {
        return;
    }

    json save;
    save["version"] = SAVE_VERSION;
    save["current_level"] = levelOverride >= 0 ? levelOverride : currentLevel_;
    save["score"] = score_;
    save["lives"] = lives_;
    save["powerups"] = {
        {"paddle_extend_remaining", std::max(paddle1_.extendTimer, paddle2_.extendTimer)},
        {"slow_ball_remaining", std::max(0.0f, slowBallTimer_)}
    };
    save["campaign"] = true;

    std::ofstream file(SAVE_PATH);
    if (!file.is_open()) {
        TraceLog(LOG_ERROR, "无法写入存档: %s", SAVE_PATH);
        jsonStatusMessage_ = "存档写入失败";
        return;
    }
    file << save.dump(4);
    RefreshCampaignSaveFlag();
}

// 反序列化 save.json；v1 自动升级写回 v2
bool Game::LoadGame() {
    if (!SaveFileExists(SAVE_PATH)) {
        return false;
    }

    try {
        std::ifstream file(SAVE_PATH);
        json save;
        file >> save;

        int version = save.value("version", 1);
        if (version > SAVE_VERSION) {
            TraceLog(LOG_WARNING, "存档版本过新 (%d)，使用默认值", version);
            return false;
        }

        currentLevel_ = save.value("current_level", 1);
        score_ = save.value("score", 0);
        lives_ = save.value("lives", config_.value("game", json::object()).value("max_lives", 5));

        if (lives_ < 1) {
            TraceLog(LOG_WARNING, "存档生命值为 0，无法继续");
            return false;
        }
        if (currentLevel_ < 1) currentLevel_ = 1;
        if (currentLevel_ > maxLevel_) currentLevel_ = maxLevel_;

        const json powerups = save.value("powerups", json::object());
        const float paddleExtend = powerups.value("paddle_extend_remaining", 0.0f);
        slowBallTimer_ = powerups.value("slow_ball_remaining", 0.0f);

        paddle1_.extendTimer = paddleExtend;
        paddle2_.extendTimer = paddleExtend;
        if (paddleExtend > 0.0f) {
            const float extra = config_.value("powerups", json::object())
                                    .value("paddle_extend", json::object())
                                    .value("extra_width", 40.0f);
            paddle1_.width = paddle1_.originalWidth + extra;
            paddle2_.width = paddle2_.originalWidth + extra;
        }

        if (version == 1) {
            TraceLog(LOG_INFO, "旧版存档(v1)已迁移到 v%d", SAVE_VERSION);
            SaveProgress();
        }

        if (!save.value("campaign", false)) {
            TraceLog(LOG_WARNING, "存档非战役模式，无法 Continue");
            return false;
        }

        jsonStatusMessage_ = TextFormat("读档成功：关卡 %d", currentLevel_);
        return true;
    } catch (const std::exception& e) {
        TraceLog(LOG_ERROR, "读档失败: %s", e.what());
        jsonStatusMessage_ = "存档损坏，已忽略";
        return false;
    }
}

void Game::DeleteSave() {
    std::remove(SAVE_PATH);
    RefreshCampaignSaveFlag();
}

LevelData Game::BuildRandomLevelData() {
    LevelData data;
    data.backgroundTexturePath = "assets/bg_level1.png";
    data.brickTexturePath = "assets/brick_level1.png";
    data.hitSoundPath = "assets/hit1.wav";

    const float bWidth = config_.value("bricks", json::object()).value("width", 70.0f);
    const float bHeight = config_.value("bricks", json::object()).value("height", 18.0f);
    const float spacing = config_.value("bricks", json::object()).value("spacing", 4.0f);
    const int cols = 7 + rand() % 4;
    const int rows = 4 + rand() % 3;
    const float offsetX = 40.0f + static_cast<float>(rand() % 40);
    const float offsetY = 50.0f + static_cast<float>(rand() % 30);

    const Color palette[] = {RED, ORANGE, GOLD, GREEN, SKYBLUE, PINK, YELLOW};

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            if (rand() % 100 > 62) {
                continue;
            }
            data.bricks.emplace_back(
                Vector2{offsetX + static_cast<float>(c) * (bWidth + spacing),
                        offsetY + static_cast<float>(r) * (bHeight + spacing)},
                bWidth,
                bHeight,
                palette[rand() % 7]
            );
        }
    }

    return data;
}

void Game::StartRandomLevelLoad() {
    if (isLoading_) {
        return;
    }

    pendingLoadLevel_ = 0;
    isLoading_ = true;
    loadReady_ = false;
    loadFailed_ = false;
    loadingMessage_ = "Generating";

    loadFuture_ = std::async(std::launch::async, [this]() {
        return BuildRandomLevelData();
    });
}

// 关卡编辑器：E 切换；左键加砖、右键删砖、S 导出 levels/custom.json
void Game::UpdateEditor() {
    if (IsKeyPressed(KEY_S)) {
        SaveLayoutToJson("levels/custom.json");
        jsonStatusMessage_ = "已保存到 levels/custom.json";
    }

    const float stepX = editorBrickWidth_ + editorSpacing_;
    const float stepY = editorBrickHeight_ + editorSpacing_;

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        const Vector2 mouse = GetMousePosition();
        const float x = std::floor(mouse.x / stepX) * stepX;
        const float y = std::floor(mouse.y / stepY) * stepY;
        const Rectangle cell = {x, y, editorBrickWidth_, editorBrickHeight_};

        for (const auto& brick : bricks_) {
            if (brick.active && CheckCollisionRecs(cell, brick.rect)) {
                return;
            }
        }

        bricks_.emplace_back(Vector2{x, y}, editorBrickWidth_, editorBrickHeight_, ORANGE);
        RebuildCollisionGrid();
    }

    if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
        const Vector2 mouse = GetMousePosition();
        for (auto& brick : bricks_) {
            if (!brick.active) {
                continue;
            }
            if (CheckCollisionPointRec(mouse, brick.rect)) {
                brick.active = false;
                --activeBrickCount_;
                RebuildCollisionGrid();
                break;
            }
        }
    }
}

void Game::DrawEditorOverlay() {
    if (!editingMode_) {
        return;
    }

    DrawRectangle(0, 0, screenWidth_, 36, Fade(BLACK, 0.65f));
    DrawText("EDIT MODE  |  LMB:add  RMB:del  S:save  E:exit  ESC:pause",
             10, 10, 16, YELLOW);

    const float stepX = editorBrickWidth_ + editorSpacing_;
    const float stepY = editorBrickHeight_ + editorSpacing_;
    for (float x = 0.0f; x < screenWidth_; x += stepX) {
        DrawLineV({x, 0.0f}, {x, (float)screenHeight_}, Fade(SKYBLUE, 0.2f));
    }
    for (float y = 0.0f; y < screenHeight_; y += stepY) {
        DrawLineV({0.0f, y}, {(float)screenWidth_, y}, Fade(SKYBLUE, 0.2f));
    }
}

void Game::SaveLayoutToJson(const std::string& path) {
    if (bricks_.empty()) {
        TraceLog(LOG_WARNING, "没有砖块可保存");
        return;
    }

    float minX = bricks_[0].rect.x;
    float minY = bricks_[0].rect.y;
    float maxX = minX;
    float maxY = minY;
    float bW = bricks_[0].rect.width;
    float bH = bricks_[0].rect.height;

    for (const auto& brick : bricks_) {
        if (!brick.active) {
            continue;
        }
        minX = std::min(minX, brick.rect.x);
        minY = std::min(minY, brick.rect.y);
        maxX = std::max(maxX, brick.rect.x);
        maxY = std::max(maxY, brick.rect.y);
        bW = brick.rect.width;
        bH = brick.rect.height;
    }

    const float spacing = editorSpacing_;
    const int cols = static_cast<int>((maxX - minX) / (bW + spacing)) + 1;
    const int rows = static_cast<int>((maxY - minY) / (bH + spacing)) + 1;

    json pattern = json::array();
    for (int r = 0; r < rows; ++r) {
        std::string row;
        row.reserve(static_cast<size_t>(cols));
        for (int c = 0; c < cols; ++c) {
            const Rectangle cell = {
                minX + static_cast<float>(c) * (bW + spacing),
                minY + static_cast<float>(r) * (bH + spacing),
                bW,
                bH
            };
            bool found = false;
            for (const auto& brick : bricks_) {
                if (brick.active && CheckCollisionRecs(cell, brick.rect)) {
                    row.push_back('R');
                    found = true;
                    break;
                }
            }
            if (!found) {
                row.push_back('.');
            }
        }
        pattern.push_back(row);
    }

    json out = {
        {"background", "assets/bg_level1.png"},
        {"brick_texture", "assets/brick_level1.png"},
        {"hit_sound", "assets/hit1.wav"},
        {"brick_width", bW},
        {"brick_height", bH},
        {"spacing", spacing},
        {"offset_x", minX},
        {"offset_y", minY},
        {"pattern", pattern}
    };

    std::ofstream file(path);
    if (file.is_open()) {
        file << out.dump(4);
        TraceLog(LOG_INFO, "关卡布局已保存: %s", path.c_str());
    }
}

void Game::StartLevelLoad(int level) {
    if (level > maxLevel_) {
        DeleteSave();
        hasPendingSave_ = false;
        state_ = GameState::GAME_OVER;
        SaveScore();
        return;
    }

    if (isLoading_) {
        return;
    }

    pendingLoadLevel_ = level;
    isLoading_ = true;
    loadReady_ = false;
    loadFailed_ = false;
    loadingMessage_ = "Loading";

    loadFuture_ = std::async(std::launch::async, [this, level]() {
        return BuildLevelData(level);
    });
}

LevelData Game::BuildLevelData(int level) {
    const std::string path = "levels/level" + std::to_string(level) + ".json";
    const json levelJson = LoadJSONWithFallback(path, GetDefaultLevelJson(level));
    return ParseLevelJson(levelJson, level);
}

void Game::ApplyLoadedLevel(const LevelData& data, int level) {
    bricks_.clear();
    bricks_ = data.bricks;

    if (!bricks_.empty()) {
        editorBrickWidth_ = bricks_[0].rect.width;
        editorBrickHeight_ = bricks_[0].rect.height;
    }

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

    if (!isRandomMode_) {
        currentLevel_ = std::min(level, maxLevel_);
        SaveProgress();
    }
    RebuildCollisionGrid();
    loadReady_ = true;
    isLoading_ = false;
    loadFailed_ = false;
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