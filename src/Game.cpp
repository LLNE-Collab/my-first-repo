#include "Game.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <ctime>

// ----- Ball -----
Ball::Ball(Vector2 pos, Vector2 vel, float r, Color c)
    : position(pos), velocity(vel), radius(r), color(c), trail() {}
Ball::~Ball() {}

void Ball::Update(float dt) {
    trail.push_back({position, 0.3f});
    if (trail.size() > 10) trail.pop_front();
    for (auto& t : trail) t.life -= dt;

    position.x += velocity.x * dt * 60.0f;
    position.y += velocity.y * dt * 60.0f;
}

void Ball::Draw() const {
    for (size_t i = 0; i < trail.size(); ++i) {
        float alpha = trail[i].life / 0.3f;
        Color trailColor = Fade(color, alpha * 0.5f);
        DrawCircleV(trail[i].pos, radius * (0.5f + i * 0.05f), trailColor);
    }
    DrawCircleV(position, radius, color);
}

// ----- Paddle -----
Paddle::Paddle(Vector2 pos, float w, float h, Color c)
    : position(pos), width(w), height(h), color(c), originalWidth(w), extendTimer(0) {}
Paddle::~Paddle() {}

void Paddle::Extend(float extraWidth, float duration) {
    width += extraWidth;
    extendTimer = duration;
}

void Paddle::Update(float dt) {
    if (extendTimer > 0) {
        extendTimer -= dt;
        if (extendTimer <= 0) width = originalWidth;
    }
}

void Paddle::Draw() const {
    DrawRectangleRec({position.x, position.y, width, height}, color);
    if (width > originalWidth) {
        DrawRectangleLinesEx({position.x - 2, position.y - 2, width + 4, height + 4}, 2, Fade(GREEN, 0.3f));
    }
}

// ----- Brick -----
Brick::Brick(Vector2 pos, float w, float h, Color c)
    : rect({pos.x, pos.y, w, h}), active(true), color(c) {}
Brick::~Brick() {}

void Brick::Draw() const {
    if (active) {
        DrawRectangleRec(rect, color);
        DrawRectangleLinesEx(rect, 1, Fade(BLACK, 0.3f));
    }
}

// ----- PowerUp -----
PowerUp::PowerUp(Vector2 pos, PowerUpType t)
    : position(pos), type(t), active(true), duration(5.0f) {}
PowerUp::~PowerUp() {}

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
    float rotation = GetTime() * 2.0f;
    DrawCircleSector(position, 12, rotation, rotation + 180, 3, Fade(color, 0.3f));
}

// ----- Particle -----
Particle::Particle(Vector2 p, Vector2 v, Color c, float l)
    : pos(p), vel(v), color(c), life(l) {}
Particle::~Particle() {}

void Particle::Update(float dt) {
    pos.x += vel.x * dt;
    pos.y += vel.y * dt;
    life -= dt;
}
void Particle::Draw() {
    if (life > 0) {
        float alpha = life / 0.5f;
        DrawCircleV(pos, 2, Fade(color, alpha));
    }
}

// ----- PowerUpEffect -----
ExtendPaddleEffect::ExtendPaddleEffect(float w, float d) : extraWidth(w), duration(d) {}
ExtendPaddleEffect::~ExtendPaddleEffect() {}
void ExtendPaddleEffect::Apply(Game& game) {
    game.ApplyPaddleExtend(extraWidth, duration);
}

MultiBallEffect::MultiBallEffect(int b) : extraBalls(b) {}
MultiBallEffect::~MultiBallEffect() {}
void MultiBallEffect::Apply(Game& game) {
    for (int i = 0; i < extraBalls; ++i) {
        Ball newBall = game.GetBall();
        newBall.velocity.x = (float)((rand() % 200 - 100) / 50.0f);
        newBall.velocity.y = -fabs(newBall.velocity.y);
        game.AddBall(newBall);
    }
}

SlowBallEffect::SlowBallEffect(float f, float d) : speedFactor(f), duration(d) {}
SlowBallEffect::~SlowBallEffect() {}
void SlowBallEffect::Apply(Game& game) {
    game.ApplySlowBall(speedFactor, duration);
}

// PowerUpEffect factory function
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

// ----- Game 实现 -----
Game::Game()
    : screenWidth_(800), screenHeight_(600),
      state_(GameState::MENU), score_(0), lives_(5), level_(1),
      paddle_(Vector2{400, 550}, 100.0f, 10.0f, BLUE),
      paddleExtendTimer_(0), slowBallTimer_(0), ballRespawnTimer_(0) {
    srand((unsigned)time(nullptr));
    LoadConfig();

    screenWidth_ = config_["screen"]["width"];
    screenHeight_ = config_["screen"]["height"];
    
    InitWindow(screenWidth_, screenHeight_, config_["screen"]["title"].get<std::string>().c_str());
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
    CloseWindow();
}

void Game::Run() {
    while (!WindowShouldClose()) {
        Update();
        Draw();
    }
}

void Game::Update() {
    switch(state_) {
        case GameState::MENU: UpdateMenu(); break;
        case GameState::PLAYING: UpdatePlaying(); break;
        case GameState::PAUSED: UpdatePaused(); break;
        case GameState::GAME_OVER: if (IsKeyPressed(KEY_ENTER)) state_ = GameState::MENU; break;
        case GameState::LEADERBOARD: if (IsKeyPressed(KEY_ESCAPE)) state_ = GameState::MENU; break;
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

void Game::UpdatePlaying() {
    float dt = GetFrameTime();

    // 更新挡板扩展效果计时器
    if (paddleExtendTimer_ > 0.0f) {
        paddleExtendTimer_ -= dt;
        if (paddleExtendTimer_ <= 0.0f) {
            paddle_.width = paddle_.originalWidth;
        }
    }

    // 更新慢球效果计时器
    if (slowBallTimer_ > 0.0f) {
        slowBallTimer_ -= dt;
        if (slowBallTimer_ <= 0.0f) {
            for (size_t i = 0; i < balls_.size() && i < originalVelocities_.size(); ++i) {
                balls_[i].velocity = originalVelocities_[i];
            }
        }
    }

    // 球重生计时器
    if (ballRespawnTimer_ > 0.0f) {
        ballRespawnTimer_ -= dt;
        if (ballRespawnTimer_ <= 0.0f && balls_.empty()) {
            float x = (rand() % (screenWidth_ - 40)) + 20.0f;
            float y = 200.0f + (rand() % 300);
            float vx = (rand() % 200 - 100) / 10.0f;
            float vy = (rand() % 200 - 100) / 10.0f;
            if (fabs(vx) < 5.0f) vx = (vx >= 0 ? 5.0f : -5.0f);
            if (fabs(vy) < 5.0f) vy = (vy >= 0 ? 5.0f : -5.0f);
            balls_.push_back(Ball({x, y}, {vx, vy}, 10, RED));
        }
    }

    // 更新所有道具状态及删除失效道具
    for (auto& p : powerUps_) p.Update(dt, (float)screenHeight_);
    powerUps_.erase(std::remove_if(powerUps_.begin(),
                                   powerUps_.end(),
                                   [](const PowerUp& p) { return !p.active; }),
                    powerUps_.end());

    // 更新粒子效果及清理
    for (auto& part : particles_) part.Update(dt);
    particles_.erase(std::remove_if(particles_.begin(),
                                    particles_.end(),
                                    [](const Particle& part) { return part.life <= 0.0f; }),
                     particles_.end());

    // WASD控制挡板移动
    if (IsKeyDown(KEY_W)) paddle_.position.y -= 500.0f * dt;
    if (IsKeyDown(KEY_A)) paddle_.position.x -= 500.0f * dt;
    if (IsKeyDown(KEY_S)) paddle_.position.y += 500.0f * dt;
    if (IsKeyDown(KEY_D)) paddle_.position.x += 500.0f * dt;

    // 挡板边界检测，限制在屏幕内
    if (paddle_.position.x < 0) paddle_.position.x = 0;
    if (paddle_.position.x + paddle_.width > screenWidth_)
        paddle_.position.x = screenWidth_ - paddle_.width;

    // 球的边界反弹检查
    for (auto& ball : balls_) {
        if (ball.position.x - ball.radius <= 0 || ball.position.x + ball.radius >= screenWidth_)
            ball.velocity.x *= -1;
        if (ball.position.y - ball.radius <= 0)
            ball.velocity.y *= -1;
    }

    // 多球循环更新及碰撞检测
    for (size_t i = 0; i < balls_.size();) {
        Ball& ball = balls_[i];
        ball.Update(dt);

        // 球与挡板碰撞
        Rectangle paddleRect = {paddle_.position.x, paddle_.position.y, paddle_.width, paddle_.height};
        if (CheckCollisionCircleRec(ball.position, ball.radius, paddleRect) && ball.velocity.y > 0) {
            ball.velocity.y *= -1;
            float hitPos = (ball.position.x - paddle_.position.x) / paddle_.width;
            ball.velocity.x = 8 * (hitPos - 0.5f);
        }

        // 球与砖块碰撞
        for (auto& brick : bricks_) {
            if (brick.active && CheckCollisionCircleRec(ball.position, ball.radius, brick.rect)) {
                brick.active = false;
                ball.velocity.y *= -1;
                score_ += 1;

                // 生成粒子效果
                for (int j = 0; j < 20; ++j) {
                    particles_.push_back(Particle(
                        Vector2{brick.rect.x + (float)(rand() % (int)brick.rect.width),
                                brick.rect.y + (float)(rand() % (int)brick.rect.height)},
                        Vector2{(float)((rand() % 200 - 100) / 10.0f),
                                (float)((rand() % 200 - 100) / 10.0f)},
                        brick.color,
                        0.5f));
                }

                // 30%概率随机生成道具
                if ((rand() % 100) < 30) {
                    PowerUpType type = static_cast<PowerUpType>(rand() % 3);
                    PowerUp powerUp({brick.rect.x + brick.rect.width / 2, brick.rect.y}, type);
                    powerUp.duration = 5.0f;
                    powerUps_.push_back(powerUp);
                }

                // 判断是否所有砖块被打完
                bool allBricksDestroyed = std::all_of(bricks_.begin(), bricks_.end(),
                    [](const Brick& b){ return !b.active; });

                if (allBricksDestroyed) {
                    state_ = GameState::GAME_OVER;
                    SaveScore();
                }
                break; // 一次只处理一个砖块碰撞
            }
        }

        // ** 修改点：用挡板和道具的矩形碰撞检测来判断是否接住道具 **
        for (auto& powerUp : powerUps_) {
            if (powerUp.active) {
                Rectangle powerUpRect = {powerUp.position.x - 10, powerUp.position.y - 5, 20, 10};
                if (CheckCollisionRecs(paddleRect, powerUpRect)) {
                    powerUp.active = false;
                    auto effect = CreatePowerUpEffect(powerUp.type, config_);
                    if (effect) effect->Apply(*this);
                    if (powerUp.type == PowerUpType::PADDLE_EXTEND) paddleExtendTimer_ = 5.0f;
                    else if (powerUp.type == PowerUpType::SLOW_BALL) slowBallTimer_ = 5.0f;
                }
            }
        }

        // 球掉出屏幕下方时移除并扣命
        if (ball.position.y + ball.radius >= screenHeight_) {
            balls_.erase(balls_.begin() + i);
            if (balls_.empty()) {
                lives_--;
                if (lives_ <= 0) {
                    state_ = GameState::GAME_OVER;
                    SaveScore();
                } else {
                    ballRespawnTimer_ = 2.0f;
                }
            }
        } else {
            i++;
        }
    }

    // 按ESC暂停
    if (IsKeyPressed(KEY_ESCAPE)) state_ = GameState::PAUSED;
}

void Game::UpdatePaused() {
    if (IsKeyPressed(KEY_C)) state_ = GameState::PLAYING;
    if (IsKeyPressed(KEY_Q)) state_ = GameState::MENU;
}

void Game::ResetGame() {
    lives_ = config_["game"]["max_lives"].get<int>();
    score_ = 0;

    float speed = config_["ball"]["speed_base"].get<float>();

    balls_.clear();
    balls_.push_back(Ball({ screenWidth_/2.0f, screenHeight_/2.0f}, {speed, -speed}, config_["ball"]["radius"].get<float>(), RED));

    paddle_.width = config_["paddle"]["width"].get<float>();
    paddle_.height = config_["paddle"]["height"].get<float>();
    paddle_.originalWidth = paddle_.width;
    paddle_.position = { (float)(screenWidth_ - paddle_.width)/2, (float)screenHeight_ - 50 };
    paddle_.color = BLUE;

    bricks_.clear();
    int rows = config_["bricks"]["rows"].get<int>();
    int cols = config_["bricks"]["cols"].get<int>();
    float bWidth = config_["bricks"]["width"].get<float>();
    float bHeight = config_["bricks"]["height"].get<float>();
    float spacing = config_["bricks"]["spacing"].get<float>();
    float offsetX = config_["bricks"]["offset_x"].get<float>();
    float offsetY = config_["bricks"]["offset_y"].get<float>();

    for (int r = 0; r < rows; r++) {
        for(int c=0; c<cols; c++){
            bricks_.push_back(Brick(
                {offsetX + c*(bWidth+spacing), offsetY + r*(bHeight+spacing)},
                bWidth, bHeight,
                {(unsigned char)(r*40), (unsigned char)(c*20), 100, 255}
            ));
        }
    }
    powerUps_.clear();
    particles_.clear();
    paddleExtendTimer_ = 0.0f;
    slowBallTimer_ = 0.0f;
    ballRespawnTimer_ = 0.0f;
    originalVelocities_.clear();
}

void Game::ApplyPaddleExtend(float extraWidth, float duration) {
    paddle_.Extend(extraWidth, duration);
    paddleExtendTimer_ = duration;
}

void Game::ApplySlowBall(float speedFactor, float duration) {
    originalVelocities_.clear();
    for (auto& ball : balls_) {
        originalVelocities_.push_back(ball.velocity);
        ball.velocity.x *= speedFactor;
        ball.velocity.y *= speedFactor;
    }
    slowBallTimer_ = duration;
}

void Game::AddBall(const Ball& ball) {
    balls_.push_back(ball);
}

Ball Game::GetBall() const {
    return balls_.empty() ? Ball({0, 0}, {0, 0}, 0, BLACK) : balls_[0];
}

// 读取配置
void Game::LoadConfig() {
    std::ifstream file("config.json");
    if (file.is_open()) {
        file >> config_;
    } else {
        // 默认配置
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

// 载入排行榜
void Game::LoadLeaderboard() {
    leaderboardData_.clear();
    std::ifstream file("leaderboard.txt");
    if (!file.is_open()) return;

    std::string line;
    while(std::getline(file, line)) {
        size_t pos = line.find(' ');
        if (pos != std::string::npos) {
            std::string name = line.substr(0, pos);
            int sc = std::stoi(line.substr(pos +1));
            leaderboardData_.emplace_back(name, sc);
        }
    }
    std::sort(leaderboardData_.begin(), leaderboardData_.end(), [](auto& a, auto& b){
        return a.second > b.second;
    });
}

// 保存分数进排行榜
void Game::SaveScore() {
    leaderboardData_.push_back({username_, score_});
    std::sort(leaderboardData_.begin(), leaderboardData_.end(), [](auto& a, auto& b){
        return a.second > b.second;
    });
    std::ofstream file("leaderboard.txt");
    if (!file.is_open()) return;
    for (auto& p : leaderboardData_) {
        file << p.first << " " << p.second << "\n";
    }
}

// UI绘制
void Game::DrawUI() {
    DrawText(TextFormat("Score: %d", score_), 10, 10, 20, BLACK);
    DrawText(TextFormat("Lives: %d", lives_), 10, 40, 20, BLACK);
    if (paddleExtendTimer_ > 0) {
        DrawText(TextFormat("Paddle Extend: %.1f", paddleExtendTimer_), 10, 70, 20, GREEN);
    }
    if (slowBallTimer_ > 0) {
        DrawText(TextFormat("Slow Ball: %.1f", slowBallTimer_), 10, 100, 20, YELLOW);
    }
}

// 画面绘制
void Game::Draw() {
    BeginDrawing();
    ClearBackground(RAYWHITE);

    switch(state_) {
    case GameState::MENU:
        DrawRectangle(0, 0, screenWidth_, screenHeight_, Fade(BLACK, 0.3f));
        DrawText("BREAKOUT", screenWidth_/2-80, 100, 40, BLACK);

        Color color;
        for (auto& btn : {&btnPlay_, &btnSettings_, &btnQuit_}) {
            bool hover = CheckCollisionPointRec(GetMousePosition(), *btn);
            float scale = hover ? 1.1f : 1.0f;
            color = hover ? Color{255, 100, 100, 255} : LIGHTGRAY;
            if (btn == &btnSettings_) color = hover ? Color{100, 255, 100, 255} : LIGHTGRAY;
            if (btn == &btnQuit_) color = hover ? Color{100, 100, 255, 255} : LIGHTGRAY;

            Rectangle scaledRect = {
                btn->x - btn->width*(scale - 1)/2,
                btn->y - btn->height*(scale - 1)/2,
                btn->width * scale,
                btn->height * scale
            };
            DrawRectangleRec(scaledRect, color);
            const char* txt = (btn == &btnPlay_) ? "PLAY" : (btn == &btnSettings_) ? "SETTINGS" : "QUIT";
            int xOffset = (btn == &btnPlay_) ? 70 : (btn == &btnSettings_) ? 55 : 75;
            DrawText(txt, (int)(scaledRect.x + xOffset * scale), (int)(scaledRect.y + 10 * scale), (int)(20 * scale), BLACK);
        }
        break;

    case GameState::PLAYING:
        for (auto& ball : balls_) ball.Draw();
        paddle_.Draw();
        for (auto& brick : bricks_) brick.Draw();
        for (auto& p : powerUps_) p.Draw();
        for (auto& part : particles_) part.Draw();
        DrawUI();
        break;

    case GameState::PAUSED:
        DrawUI();
        DrawRectangle(0, 0, screenWidth_, screenHeight_, {0, 0, 0, 150});
        DrawRectangle(screenWidth_/2-100, screenHeight_/2-50, 200, 100, LIGHTGRAY);
        DrawText("PAUSED", screenWidth_/2-40, screenHeight_/2-40, 20, BLACK);
        DrawText("Continue (C) / Quit (Q)", screenWidth_/2-100, screenHeight_/2, 15, BLACK);
        break;

    case GameState::GAME_OVER: {
        DrawText("GAME OVER", screenWidth_/2-80, 200, 40, RED);
        DrawText(TextFormat("Score: %d", score_), screenWidth_/2-80, 260, 20, BLACK);
        int rank = 1;
        for (auto& p : leaderboardData_) if (p.second > score_) rank++;
        DrawText(TextFormat("Rank: %d", rank), screenWidth_/2-80, 300, 20, BLACK);
        DrawText("Press ENTER to Menu", screenWidth_/2-100, 400, 20, GRAY);
        break;
    }

    case GameState::LEADERBOARD: {
        DrawRectangle(0, 0, screenWidth_, screenHeight_, Fade(BLACK, 0.3f));
        DrawText("LEADERBOARD", screenWidth_/2-80, 100, 30, BLACK);
        int y = 150;
        for (size_t i = 0; i < leaderboardData_.size() && i < 10; i++) {
            DrawText(TextFormat("%d. %s - %d", (int)i + 1, leaderboardData_[i].first.c_str(), leaderboardData_[i].second), screenWidth_/2-100, y, 20, BLACK);
            y += 30;
        }
        DrawText("Press ESC to return", screenWidth_/2-80, 400, 20, GRAY);
        break;
    }
    }

    EndDrawing();
}

bool Game::IsButtonClicked(const Rectangle& btn) {
    return IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(GetMousePosition(), btn);
}