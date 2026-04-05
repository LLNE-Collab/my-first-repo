#include "Game.h"
#include <fstream>
#include <algorithm>
#include <cmath>

Game::Game() : state_(GameState::MENU), score_(0), lives_(5), level_(1) {
    LoadConfig();
    
    screenWidth_ = config_["screen"]["width"];
    screenHeight_ = config_["screen"]["height"];
    
    InitWindow(screenWidth_, screenHeight_, config_["screen"]["title"].get<std::string>().c_str());
    SetTargetFPS(60);
    SetExitKey(0); // 禁用 Raylib 默认 ESC 退出行为，改为自己处理按键逻辑
    
    // 初始化按钮区域 (居中布局)
    int btnW = 200, btnH = 40;
    int startX = screenWidth_/2 - btnW/2;
    int startY = 200;
    btnPlay_ = { (float)startX, (float)startY, btnW, btnH };
    btnSettings_ = { (float)startX, (float)startY + 50, btnW, btnH };
    btnLevelSelect_ = { (float)startX, (float)startY + 100, btnW, btnH };
    btnLeaderboard_ = { (float)startX, (float)startY + 150, btnW, btnH };
    btnQuit_ = { (float)startX, (float)startY + 200, btnW, btnH };
    
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
    switch (state_) {
        case GameState::MENU: UpdateMenu(); break;
        case GameState::NAME_INPUT: UpdateNameInput(); break;
        case GameState::LEVEL_SELECT: UpdateLevelSelect(); break;
        case GameState::SETTINGS: UpdateSettings(); break;
        case GameState::PLAYING: UpdatePlaying(); break;
        case GameState::PAUSED: UpdatePaused(); break;
        case GameState::GAME_OVER: UpdateGameOver(); break;
        case GameState::LEADERBOARD: UpdateLeaderboard(); break;
        case GameState::WIN: /* 类似 GAME_OVER */ break;
    }
}

void Game::Draw() {
    BeginDrawing();
    ClearBackground(RAYWHITE);

    switch (state_) {
        case GameState::MENU: {
            DrawRectangle(0, 0, screenWidth_, screenHeight_, Fade(BLACK, 0.3f)); // 模糊背景模拟
            DrawText("BREAKOUT", screenWidth_/2 - 80, 100, 40, BLACK);
            DrawRectangleRec(btnPlay_, LIGHTGRAY); DrawText("PLAY", btnPlay_.x + 70, btnPlay_.y + 10, 20, BLACK);
            DrawRectangleRec(btnSettings_, LIGHTGRAY); DrawText("SETTINGS", btnSettings_.x + 55, btnSettings_.y + 10, 20, BLACK);
            DrawRectangleRec(btnLevelSelect_, LIGHTGRAY); DrawText("LEVELS", btnLevelSelect_.x + 65, btnLevelSelect_.y + 10, 20, BLACK);
            DrawRectangleRec(btnLeaderboard_, LIGHTGRAY); DrawText("LEADERBOARD", btnLeaderboard_.x + 35, btnLeaderboard_.y + 10, 20, BLACK);
            DrawRectangleRec(btnQuit_, LIGHTGRAY); DrawText("QUIT", btnQuit_.x + 75, btnQuit_.y + 10, 20, BLACK);
            break;
        }

        case GameState::NAME_INPUT: {
            DrawText("Enter Your Name:", screenWidth_/2 - 100, 200, 20, BLACK);
            DrawRectangle(screenWidth_/2 - 100, 250, 200, 40, LIGHTGRAY);
            DrawText(username_.c_str(), screenWidth_/2 - 90, 260, 20, BLACK);
            DrawText("Press ENTER to confirm", screenWidth_/2 - 110, 320, 20, GRAY);
            break;
        }

        case GameState::PLAYING: {
            // 绘制游戏对象
            DrawCircleV(ball_.pos, ball_.radius, ball_.color);
            DrawRectangleV(ball_.pos, {ball_.radius, ball_.radius}, ball_.color); // 调试用，实际画圆
            DrawRectangleRec({paddle_.pos.x, paddle_.pos.y, paddle_.width, paddle_.height}, paddle_.color);
            
            for (const auto& b : bricks_) {
                if (b.active) DrawRectangleRec(b.rect, b.color);
            }
            DrawUI();
            break;
        }

        case GameState::PAUSED: {
            DrawUI(); // 先画游戏背景
            DrawRectangle(0, 0, screenWidth_, screenHeight_, {0, 0, 0, 150}); // 半透明遮罩
            DrawRectangle(screenWidth_/2 - 100, screenHeight_/2 - 50, 200, 100, LIGHTGRAY);
            DrawText("PAUSED", screenWidth_/2 - 40, screenHeight_/2 - 40, 20, BLACK);
            DrawText("Continue (C) / Quit (Q)", screenWidth_/2 - 100, screenHeight_/2, 15, BLACK);
            break;
        }
            
        case GameState::GAME_OVER: {
            DrawText("GAME OVER", screenWidth_/2 - 80, 200, 40, RED);
            DrawText(TextFormat("Player: %s", username_.c_str()), screenWidth_/2 - 80, 260, 20, BLACK);
            DrawText(TextFormat("Score: %d", score_), screenWidth_/2 - 80, 290, 20, BLACK);
            // 简单排名显示
            int rank = 1;
            for(auto& p : leaderboardData_) if(p.second > score_) rank++;
            DrawText(TextFormat("Rank: %d", rank), screenWidth_/2 - 80, 320, 20, BLACK);
            DrawText("Press ENTER to Menu", screenWidth_/2 - 100, 400, 20, GRAY);
            break;
        }

        case GameState::SETTINGS: {
            DrawRectangle(0, 0, screenWidth_, screenHeight_, Fade(BLACK, 0.3f));
            DrawText("SETTINGS", screenWidth_/2 - 60, 100, 30, BLACK);
            DrawText("Ball Color:", screenWidth_/2 - 100, 200, 20, BLACK);
            DrawRectangle(screenWidth_/2 - 70, 220, 50, 50, RED); DrawText("R", screenWidth_/2 - 45, 240, 20, WHITE);
            DrawRectangle(screenWidth_/2 - 10, 220, 50, 50, GREEN); DrawText("G", screenWidth_/2 + 15, 240, 20, WHITE);
            DrawRectangle(screenWidth_/2 + 50, 220, 50, 50, BLUE); DrawText("B", screenWidth_/2 + 75, 240, 20, WHITE);
            DrawText("Press ESC to return", screenWidth_/2 - 80, 400, 20, GRAY);
            break;
        }

        case GameState::LEVEL_SELECT: {
            DrawRectangle(0, 0, screenWidth_, screenHeight_, Fade(BLACK, 0.3f));
            DrawText("SELECT LEVEL", screenWidth_/2 - 80, 100, 30, BLACK);
            DrawText("1. Easy", screenWidth_/2 - 50, 200, 20, BLACK);
            DrawText("2. Medium", screenWidth_/2 - 50, 250, 20, BLACK);
            DrawText("3. Hard", screenWidth_/2 - 50, 300, 20, BLACK);
            DrawText("Press 1/2/3 to select", screenWidth_/2 - 80, 400, 20, GRAY);
            break;
        }

        case GameState::LEADERBOARD: {
            DrawRectangle(0, 0, screenWidth_, screenHeight_, Fade(BLACK, 0.3f));
            DrawText("LEADERBOARD", screenWidth_/2 - 80, 100, 30, BLACK);
            int y = 150;
            for (size_t i = 0; i < leaderboardData_.size() && i < 10; ++i) {
                DrawText(TextFormat("%d. %s - %d", (int)i+1, leaderboardData_[i].first.c_str(), leaderboardData_[i].second), screenWidth_/2 - 100, y, 20, BLACK);
                y += 30;
            }
            DrawText("Press ESC to return", screenWidth_/2 - 80, 400, 20, GRAY);
            break;
        }
    }

    EndDrawing();
}

// --- 逻辑更新实现 ---

void Game::UpdateMenu() {
    if (IsButtonClicked(btnPlay_)) {
        state_ = GameState::NAME_INPUT; 
        username_ = ""; // 清空旧名字
    }
    if (IsButtonClicked(btnSettings_)) state_ = GameState::SETTINGS;
    if (IsButtonClicked(btnQuit_)) exit(0);
    if (IsButtonClicked(btnLevelSelect_)) state_ = GameState::LEVEL_SELECT;
    if (IsButtonClicked(btnLeaderboard_)) state_ = GameState::LEADERBOARD;
}

void Game::UpdateNameInput() {
    int key = GetCharPressed();
    while (key > 0) {
        if ((key >= 32) && (key <= 125)) username_ += (char)key;
        key = GetCharPressed();
    }
    if (IsKeyPressed(KEY_BACKSPACE) && !username_.empty()) username_.pop_back();
    if (IsKeyPressed(KEY_ENTER) && !username_.empty()) {
        state_ = GameState::PLAYING;
        ResetGame();
    }
}

void Game::UpdatePlaying() {
    // 板控制 (WSAD 风格移动)
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) paddle_.pos.x -= config_["paddle"]["speed"].get<float>();
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) paddle_.pos.x += config_["paddle"]["speed"].get<float>();
    if (paddle_.pos.x < 0) paddle_.pos.x = 0;
    if (paddle_.pos.x + paddle_.width > screenWidth_) paddle_.pos.x = screenWidth_ - paddle_.width;

    // 球运动
    ball_.pos.x += ball_.vel.x;
    ball_.pos.y += ball_.vel.y;

    // 墙壁碰撞
    if (ball_.pos.x - ball_.radius <= 0 || ball_.pos.x + ball_.radius >= screenWidth_) ball_.vel.x *= -1;
    if (ball_.pos.y - ball_.radius <= 0) ball_.vel.y *= -1;

    // 板碰撞 (简易逻辑)
    Rectangle rec = {paddle_.pos.x, paddle_.pos.y, paddle_.width, paddle_.height};
    if (CheckCollisionCircleRec(ball_.pos, ball_.radius, rec) && ball_.vel.y > 0) {
        ball_.vel.y *= -1;
        float hitPos = (ball_.pos.x - paddle_.pos.x) / paddle_.width;
        ball_.vel.x = 8 * (hitPos - 0.5f);
    }

    // 砖块碰撞
    for (auto& brick : bricks_) {
        if (brick.active && CheckCollisionCircleRec(ball_.pos, ball_.radius, brick.rect)) {
            brick.active = false;
            ball_.vel.y *= -1;
            score_ += 10;
        }
    }

    // 失败检测
    if (ball_.pos.y + ball_.radius >= paddle_.pos.y + paddle_.height) {
        lives_--;
        if (lives_ <= 0) {
            state_ = GameState::GAME_OVER;
            SaveScore();
        } else {
            ball_.pos = { (float)screenWidth_/2, (float)screenHeight_/2 };
        }
    }

    // 暂停
    if (IsKeyPressed(KEY_ESCAPE)) state_ = GameState::PAUSED;
}

void Game::UpdatePaused() {
    if (IsKeyPressed(KEY_C)) state_ = GameState::PLAYING;
    if (IsKeyPressed(KEY_Q)) state_ = GameState::MENU;
}

// --- 辅助函数 ---

void Game::ResetGame() {
    lives_ = config_["game"]["max_lives"].get<int>();
    score_ = 0;
    
    // 根据关卡调整速度
    float speedMultiplier = 1.0f + (level_ - 1) * 0.2f;
    float baseSpeed = config_["ball"]["speed_base"].get<float>();
    
    ball_.pos = { (float)screenWidth_/2, (float)screenHeight_/2 };
    ball_.vel = { baseSpeed * speedMultiplier, -baseSpeed * speedMultiplier };
    ball_.radius = config_["ball"]["radius"].get<float>();
    ball_.color = JsonToColor(config_["ball"]["color"]);

    paddle_.width = config_["paddle"]["width"].get<float>();
    paddle_.height = config_["paddle"]["height"].get<float>();
    paddle_.pos = { (float)(screenWidth_ - paddle_.width)/2, (float)screenHeight_ - 50 };
    paddle_.color = JsonToColor(config_["paddle"]["color"]);

    bricks_.clear();
    int rows = config_["bricks"]["rows"].get<int>();
    int cols = config_["bricks"]["cols"].get<int>();
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            Brick b;
            b.rect.x = config_["bricks"]["offset_x"].get<float>() + c * (config_["bricks"]["width"].get<float>() + config_["bricks"]["spacing"].get<float>());
            b.rect.y = config_["bricks"]["offset_y"].get<float>() + r * (config_["bricks"]["height"].get<float>() + config_["bricks"]["spacing"].get<float>());
            b.rect.width = config_["bricks"]["width"].get<float>();
            b.rect.height = config_["bricks"]["height"].get<float>();
            b.active = true;
            b.color = { (unsigned char)(r*40), (unsigned char)(c*20), 100, 255 };
            bricks_.push_back(b);
        }
    }
}

void Game::DrawUI() {
    // 生命值 (左上角)
    for (int i = 0; i < lives_; i++) {
        // 这里用文字爱心代替，若要图形爱心需自定义绘制函数
        DrawText("♥", 10 + i * 25, 10, 20, RED);
    }
    // 分数 (右上角)
    DrawText(TextFormat("Score: %d", score_), screenWidth_ - 150, 10, 20, BLACK);
    // 用户名 (右上角)
    DrawText(username_.c_str(), screenWidth_ - 150, 40, 20, DARKGRAY);
}

bool Game::IsButtonClicked(Rectangle rec) {
    return CheckCollisionPointRec(GetMousePosition(), rec) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

Color Game::JsonToColor(const json& j) {
    return { j[0], j[1], j[2], j[3] };
}

void Game::SaveScore() {
    leaderboardData_.push_back({username_, score_});
    std::sort(leaderboardData_.begin(), leaderboardData_.end(), 
              [](const auto& a, const auto& b){ return a.second > b.second; });
    // 写入文件
    std::ofstream file("leaderboard.txt");
    if(file.is_open()) {
        for(auto& p : leaderboardData_) {
            file << p.first << " " << p.second << "\n";
        }
    }
}

void Game::LoadLeaderboard() {
    std::ifstream file("leaderboard.txt");
    if(file.is_open()) {
        std::string name;
        int sc;
        while(file >> name >> sc) {
            leaderboardData_.push_back({name, sc});
        }
    }
}

void Game::UpdateLeaderboard() {
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER)) state_ = GameState::MENU;
    // 绘制逻辑在 Draw 中处理，这里只处理退出
}

void Game::UpdateSettings() {
    Rectangle redBtn = { (float)screenWidth_/2 - 70, 220, 50, 50 };
    Rectangle greenBtn = { (float)screenWidth_/2 - 10, 220, 50, 50 };
    Rectangle blueBtn = { (float)screenWidth_/2 + 50, 220, 50, 50 };

    if (IsButtonClicked(redBtn) || IsKeyPressed(KEY_R)) ball_.color = RED;
    if (IsButtonClicked(greenBtn) || IsKeyPressed(KEY_G)) ball_.color = GREEN;
    if (IsButtonClicked(blueBtn) || IsKeyPressed(KEY_B)) ball_.color = BLUE;
    if (IsKeyPressed(KEY_ESCAPE)) state_ = GameState::MENU;
}

void Game::UpdateLevelSelect() {
    if (IsKeyPressed(KEY_ONE)) { level_ = 1; state_ = GameState::MENU; }
    if (IsKeyPressed(KEY_TWO)) { level_ = 2; state_ = GameState::MENU; }
    if (IsKeyPressed(KEY_THREE)) { level_ = 3; state_ = GameState::MENU; }
}

void Game::UpdateGameOver() {
    if (IsKeyPressed(KEY_ENTER)) state_ = GameState::MENU;
}

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
            {"game", {{"max_lives", 5}}}
        };
    }
}
