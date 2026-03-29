#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

// ==================== 常量定义 ====================
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define BRICK_ROWS 5
#define BRICK_COLS 10
#define MAX_INPUT_CHARS 20

// ==================== 枚举类型 ====================
typedef enum {
    STATE_MENU,
    STATE_SETTINGS,
    STATE_PLAYING,
    STATE_PAUSED,
    STATE_QUIT_CONFIRM,
    STATE_FINAL_CONFIRM,
    STATE_GAME_OVER,
    STATE_ENTER_NAME
} GameState;

typedef enum {
    DIFFICULTY_EASY,
    DIFFICULTY_MEDIUM,
    DIFFICULTY_HARD
} Difficulty;

// ==================== 结构体定义 ====================
typedef struct {
    Vector2 position;
    Vector2 speed;
    float radius;
    Color color;
    bool active;
} Ball;

typedef struct {
    Vector2 position;
    float width;
    float height;
    Color color;
} Paddle;

typedef struct {
    Rectangle rect;
    Color color;
    bool active;
} Brick;

typedef struct {
    Difficulty difficulty;
    Color ballColor;
    float resolutionScale;
    bool soundEnabled;
    char username[MAX_INPUT_CHARS + 1];
    int nameLength;
} GameSettings;

typedef struct {
    Ball ball;
    Paddle paddle;
    Brick bricks[BRICK_ROWS][BRICK_COLS];
    int score;
    int lives;
    int level;
    GameState state;
    GameState previousState;
    GameSettings settings;
    bool gameStarted;
    int selectedMenuItem;
    int selectedSettingsItem;
    unsigned int randomSeed;
} Game;

// ==================== 难度参数表 ====================
typedef struct {
    float ballSpeed;
    float paddleWidth;
} DifficultyParams;

static const DifficultyParams difficultyTable[3] = {
    [DIFFICULTY_EASY]   = { 4.0f, 120.0f },   // 简单：慢球速，长板子
    [DIFFICULTY_MEDIUM] = { 6.0f, 100.0f },   // 中等：中球速，中板子
    [DIFFICULTY_HARD]   = { 8.0f, 80.0f }     // 困难：快球速，短板子
};

// ==================== 颜色预设 ====================
static const Color ballColorPresets[] = {
    (Color){ 255, 255, 255, 255 },  // 白色
    (Color){ 255, 100, 100, 255 },  // 红色
    (Color){ 100, 255, 100, 255 },  // 绿色
    (Color){ 100, 100, 255, 255 },  // 蓝色
    (Color){ 255, 255, 100, 255 },  // 黄色
    (Color){ 255, 100, 255, 255 },  // 紫色
    (Color){ 100, 255, 255, 255 },  // 青色
    (Color){ 255, 165, 0, 255 }     // 橙色
};
static const int ballColorCount = sizeof(ballColorPresets) / sizeof(ballColorPresets[0]);

// ==================== 函数声明 ====================
void InitGame(Game *game);
void UpdateGame(Game *game);
void DrawGame(Game *game);
void ResetBall(Game *game);
void ResetBricks(Game *game);
void ResetGame(Game *game);

// 绘制函数
void DrawBlurredRect(Rectangle rect, int blurStrength, Color tint);
void DrawMenu(Game *game);
void DrawSettings(Game *game);
void DrawPauseMenu(Game *game);
void DrawQuitConfirm(Game *game);
void DrawFinalConfirm(Game *game);
void DrawGameOver(Game *game);
void DrawEnterName(Game *game);
void DrawPlaying(Game *game);

// 更新函数
void UpdateMenu(Game *game);
void UpdateSettings(Game *game);
void UpdatePlaying(Game *game);
void UpdatePaused(Game *game);
void UpdateQuitConfirm(Game *game);
void UpdateFinalConfirm(Game *game);
void UpdateGameOver(Game *game);
void UpdateEnterName(Game *game);

// 工具函数
Color GetRandomBrickColor(unsigned int seed, int row, int col);
void CheckCollisions(Game *game);
const char* GetDifficultyName(Difficulty diff);
const char* GetScoreRating(int score);

// ==================== 主函数 ====================
int main(void) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Breakout Game - Enhanced Edition");
    SetTargetFPS(60);
    
    Game game;
    InitGame(&game);
    
    while (!WindowShouldClose()) {
        UpdateGame(&game);
        DrawGame(&game);
    }
    
    CloseWindow();
    return 0;
}

// ==================== 初始化函数 ====================
void InitGame(Game *game) {
    srand((unsigned int)time(NULL));
    
    // 初始化设置
    game->settings.difficulty = DIFFICULTY_MEDIUM;
    game->settings.ballColor = ballColorPresets[0];
    game->settings.resolutionScale = 1.0f;
    game->settings.soundEnabled = true;
    strcpy(game->settings.username, "Player");
    game->settings.nameLength = (int)strlen(game->settings.username);
    
    // 初始化游戏状态
    game->state = STATE_MENU;
    game->previousState = STATE_MENU;
    game->score = 0;
    game->lives = 3;
    game->level = 1;
    game->gameStarted = false;
    game->selectedMenuItem = 0;
    game->selectedSettingsItem = 0;
    game->randomSeed = (unsigned int)time(NULL);
    
    // 初始化板子
    float paddleWidth = difficultyTable[game->settings.difficulty].paddleWidth;
    game->paddle.width = paddleWidth;
    game->paddle.height = 15;
    game->paddle.position.x = SCREEN_WIDTH / 2.0f - game->paddle.width / 2.0f;
    game->paddle.position.y = SCREEN_HEIGHT - 40;
    game->paddle.color = (Color){ 100, 150, 255, 255 };
    
    // 初始化球
    game->ball.radius = 8;
    game->ball.position.x = SCREEN_WIDTH / 2.0f;
    game->ball.position.y = SCREEN_HEIGHT - 60;
    game->ball.speed.x = 0;
    game->ball.speed.y = 0;
    game->ball.color = game->settings.ballColor;
    game->ball.active = false;
    
    // 初始化砖块
    ResetBricks(game);
}

void ResetBall(Game *game) {
    game->ball.position.x = game->paddle.position.x + game->paddle.width / 2.0f;
    game->ball.position.y = game->paddle.position.y - game->ball.radius - 5;
    game->ball.speed.x = 0;
    game->ball.speed.y = 0;
    game->ball.active = false;
}

void ResetBricks(Game *game) {
    float brickWidth = 70;
    float brickHeight = 25;
    float padding = 5;
    float startX = (SCREEN_WIDTH - (BRICK_COLS * (brickWidth + padding))) / 2.0f;
    float startY = 60;
    
    game->randomSeed = (unsigned int)time(NULL) + game->level * 1000;
    
    for (int row = 0; row < BRICK_ROWS; row++) {
        for (int col = 0; col < BRICK_COLS; col++) {
            game->bricks[row][col].rect.x = startX + col * (brickWidth + padding);
            game->bricks[row][col].rect.y = startY + row * (brickHeight + padding);
            game->bricks[row][col].rect.width = brickWidth;
            game->bricks[row][col].rect.height = brickHeight;
            game->bricks[row][col].color = GetRandomBrickColor(game->randomSeed, row, col);
            game->bricks[row][col].active = true;
        }
    }
}

void ResetGame(Game *game) {
    game->score = 0;
    game->lives = 3;
    game->level = 1;
    game->gameStarted = false;
    
    float paddleWidth = difficultyTable[game->settings.difficulty].paddleWidth;
    game->paddle.width = paddleWidth;
    game->paddle.position.x = SCREEN_WIDTH / 2.0f - game->paddle.width / 2.0f;
    
    game->ball.color = game->settings.ballColor;
    ResetBall(game);
    ResetBricks(game);
}

// ==================== 更新函数 ====================
void UpdateGame(Game *game) {
    switch (game->state) {
        case STATE_MENU:
            UpdateMenu(game);
            break;
        case STATE_SETTINGS:
            UpdateSettings(game);
            break;
        case STATE_PLAYING:
            UpdatePlaying(game);
            break;
        case STATE_PAUSED:
            UpdatePaused(game);
            break;
        case STATE_QUIT_CONFIRM:
            UpdateQuitConfirm(game);
            break;
        case STATE_FINAL_CONFIRM:
            UpdateFinalConfirm(game);
            break;
        case STATE_GAME_OVER:
            UpdateGameOver(game);
            break;
        case STATE_ENTER_NAME:
            UpdateEnterName(game);
            break;
    }
}

void UpdateMenu(Game *game) {
    // 上下键选择菜单项
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
        game->selectedMenuItem = (game->selectedMenuItem + 1) % 5;
    }
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
        game->selectedMenuItem = (game->selectedMenuItem - 1 + 5) % 5;
    }
    
    // Enter键确认选择
    if (IsKeyPressed(KEY_ENTER)) {
        switch (game->selectedMenuItem) {
            case 0: // 开始游戏
                game->state = STATE_PLAYING;
                ResetGame(game);
                break;
            case 1: // 选择难度
                game->settings.difficulty = (game->settings.difficulty + 1) % 3;
                break;
            case 2: // 设置
                game->state = STATE_SETTINGS;
                game->selectedSettingsItem = 0;
                break;
            case 3: // 输入用户名
                game->state = STATE_ENTER_NAME;
                break;
            case 4: // 退出
                game->state = STATE_FINAL_CONFIRM;
                break;
        }
    }
    
    // 鼠标点击支持
    Vector2 mousePos = GetMousePosition();
    Rectangle menuItems[] = {
        { SCREEN_WIDTH/2 - 100, 280, 200, 40 },
        { SCREEN_WIDTH/2 - 100, 330, 200, 40 },
        { SCREEN_WIDTH/2 - 100, 380, 200, 40 },
        { SCREEN_WIDTH/2 - 100, 430, 200, 40 },
        { SCREEN_WIDTH/2 - 100, 480, 200, 40 }
    };
    
    for (int i = 0; i < 5; i++) {
        if (CheckCollisionPointRec(mousePos, menuItems[i])) {
            game->selectedMenuItem = i;
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                switch (i) {
                    case 0:
                        game->state = STATE_PLAYING;
                        ResetGame(game);
                        break;
                    case 1:
                        game->settings.difficulty = (game->settings.difficulty + 1) % 3;
                        break;
                    case 2:
                        game->state = STATE_SETTINGS;
                        game->selectedSettingsItem = 0;
                        break;
                    case 3:
                        game->state = STATE_ENTER_NAME;
                        break;
                    case 4:
                        game->state = STATE_FINAL_CONFIRM;
                        break;
                }
            }
        }
    }
}

void UpdateSettings(Game *game) {
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
        game->selectedSettingsItem = (game->selectedSettingsItem + 1) % 4;
    }
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
        game->selectedSettingsItem = (game->selectedSettingsItem - 1 + 4) % 4;
    }
    
    // 左右键调整设置值
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT)) {
        switch (game->selectedSettingsItem) {
            case 0: // 球颜色
                {
                    int colorIndex = 0;
                    for (int i = 0; i < ballColorCount; i++) {
                        if (ColorToInt(game->settings.ballColor) == ColorToInt(ballColorPresets[i])) {
                            colorIndex = i;
                            break;
                        }
                    }
                    if (IsKeyPressed(KEY_RIGHT)) {
                        colorIndex = (colorIndex + 1) % ballColorCount;
                    } else {
                        colorIndex = (colorIndex - 1 + ballColorCount) % ballColorCount;
                    }
                    game->settings.ballColor = ballColorPresets[colorIndex];
                }
                break;
            case 1: // 音频开关
                game->settings.soundEnabled = !game->settings.soundEnabled;
                break;
            case 2: // 清晰度
                if (IsKeyPressed(KEY_RIGHT)) {
                    game->settings.resolutionScale = fminf(game->settings.resolutionScale + 0.25f, 2.0f);
                } else {
                    game->settings.resolutionScale = fmaxf(game->settings.resolutionScale - 0.25f, 0.5f);
                }
                break;
        }
    }
    
    // ESC返回菜单
    if (IsKeyPressed(KEY_ESCAPE)) {
        game->state = STATE_MENU;
    }
    
    // 鼠标支持
    Vector2 mousePos = GetMousePosition();
    Rectangle settingsItems[] = {
        { SCREEN_WIDTH/2 - 150, 250, 300, 40 },
        { SCREEN_WIDTH/2 - 150, 310, 300, 40 },
        { SCREEN_WIDTH/2 - 150, 370, 300, 40 },
        { SCREEN_WIDTH/2 - 150, 430, 300, 40 }
    };
    
    for (int i = 0; i < 4; i++) {
        if (CheckCollisionPointRec(mousePos, settingsItems[i])) {
            game->selectedSettingsItem = i;
        }
    }
    
    // 返回按钮
    Rectangle backBtn = { SCREEN_WIDTH/2 - 60, 490, 120, 40 };
    if (CheckCollisionPointRec(mousePos, backBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        game->state = STATE_MENU;
    }
}

void UpdatePlaying(Game *game) {
    // ESC键暂停
    if (IsKeyPressed(KEY_ESCAPE)) {
        game->state = STATE_PAUSED;
        return;
    }
    
    // 板子移动
    float paddleSpeed = 8.0f;
    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) {
        game->paddle.position.x -= paddleSpeed;
    }
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) {
        game->paddle.position.x += paddleSpeed;
    }
    
    // 鼠标控制板子
    game->paddle.position.x = GetMousePosition().x - game->paddle.width / 2.0f;
    
    // 板子边界检测
    if (game->paddle.position.x < 0) {
        game->paddle.position.x = 0;
    }
    if (game->paddle.position.x > SCREEN_WIDTH - game->paddle.width) {
        game->paddle.position.x = SCREEN_WIDTH - game->paddle.width;
    }
    
    // 发射球
    if (!game->ball.active) {
        game->ball.position.x = game->paddle.position.x + game->paddle.width / 2.0f;
        game->ball.position.y = game->paddle.position.y - game->ball.radius - 5;
        
        if (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            game->ball.active = true;
            float ballSpeed = difficultyTable[game->settings.difficulty].ballSpeed;
            game->ball.speed.x = (rand() % 3 - 1) * ballSpeed * 0.5f;
            game->ball.speed.y = -ballSpeed;
            game->gameStarted = true;
        }
    }
    
    // 球移动
    if (game->ball.active) {
        game->ball.position.x += game->ball.speed.x;
        game->ball.position.y += game->ball.speed.y;
        
        // 墙壁碰撞
        if (game->ball.position.x - game->ball.radius <= 0 || 
            game->ball.position.x + game->ball.radius >= SCREEN_WIDTH) {
            game->ball.speed.x *= -1;
        }
        if (game->ball.position.y - game->ball.radius <= 0) {
            game->ball.speed.y *= -1;
        }
        
        // 球掉落
        if (game->ball.position.y + game->ball.radius >= SCREEN_HEIGHT) {
            game->lives--;
            if (game->lives <= 0) {
                game->state = STATE_GAME_OVER;
            } else {
                ResetBall(game);
            }
        }
        
        // 碰撞检测
        CheckCollisions(game);
        
        // 检查是否通关
        bool allDestroyed = true;
        for (int row = 0; row < BRICK_ROWS; row++) {
            for (int col = 0; col < BRICK_COLS; col++) {
                if (game->bricks[row][col].active) {
                    allDestroyed = false;
                    break;
                }
            }
            if (!allDestroyed) break;
        }
        
        if (allDestroyed) {
            game->level++;
            game->ball.active = false;
            ResetBall(game);
            ResetBricks(game);
        }
    }
}

void UpdatePaused(Game *game) {
    if (IsKeyPressed(KEY_ESCAPE)) {
        game->state = STATE_PLAYING;
    }
    
    Vector2 mousePos = GetMousePosition();
    Rectangle continueBtn = { SCREEN_WIDTH/2 - 80, 280, 160, 45 };
    Rectangle quitBtn = { SCREEN_WIDTH/2 - 80, 340, 160, 45 };
    
    if (CheckCollisionPointRec(mousePos, continueBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        game->state = STATE_PLAYING;
    }
    if (CheckCollisionPointRec(mousePos, quitBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        game->state = STATE_QUIT_CONFIRM;
    }
}

void UpdateQuitConfirm(Game *game) {
    Vector2 mousePos = GetMousePosition();
    Rectangle yesBtn = { SCREEN_WIDTH/2 - 120, 320, 100, 45 };
    Rectangle noBtn = { SCREEN_WIDTH/2 + 20, 320, 100, 45 };
    
    if (CheckCollisionPointRec(mousePos, yesBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        game->state = STATE_MENU;
    }
    if (CheckCollisionPointRec(mousePos, noBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        game->state = STATE_PAUSED;
    }
    
    if (IsKeyPressed(KEY_Y)) {
        game->state = STATE_MENU;
    }
    if (IsKeyPressed(KEY_N) || IsKeyPressed(KEY_ESCAPE)) {
        game->state = STATE_PAUSED;
    }
}

void UpdateFinalConfirm(Game *game) {
    Vector2 mousePos = GetMousePosition();
    Rectangle yesBtn = { SCREEN_WIDTH/2 - 120, 320, 100, 45 };
    Rectangle noBtn = { SCREEN_WIDTH/2 + 20, 320, 100, 45 };
    
    if (CheckCollisionPointRec(mousePos, yesBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        CloseWindow();
        exit(0);
    }
    if (CheckCollisionPointRec(mousePos, noBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        game->state = STATE_MENU;
    }
    
    if (IsKeyPressed(KEY_Y)) {
        CloseWindow();
        exit(0);
    }
    if (IsKeyPressed(KEY_N) || IsKeyPressed(KEY_ESCAPE)) {
        game->state = STATE_MENU;
    }
}

void UpdateGameOver(Game *game) {
    Vector2 mousePos = GetMousePosition();
    Rectangle menuBtn = { SCREEN_WIDTH/2 - 100, 380, 200, 45 };
    Rectangle restartBtn = { SCREEN_WIDTH/2 - 100, 440, 200, 45 };
    
    if (CheckCollisionPointRec(mousePos, menuBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        game->state = STATE_MENU;
    }
    if (CheckCollisionPointRec(mousePos, restartBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        ResetGame(game);
        game->state = STATE_PLAYING;
    }
    
    if (IsKeyPressed(KEY_ENTER)) {
        game->state = STATE_MENU;
    }
    if (IsKeyPressed(KEY_R)) {
        ResetGame(game);
        game->state = STATE_PLAYING;
    }
}

void UpdateEnterName(Game *game) {
    int key = GetCharPressed();
    
    while (key > 0) {
        if (game->settings.nameLength < MAX_INPUT_CHARS) {
            if ((key >= 32) && (key <= 125)) {
                game->settings.username[game->settings.nameLength] = (char)key;
                game->settings.username[game->settings.nameLength + 1] = '\0';
                game->settings.nameLength++;
            }
        }
        key = GetCharPressed();
    }
    
    if (IsKeyPressed(KEY_BACKSPACE)) {
        if (game->settings.nameLength > 0) {
            game->settings.nameLength--;
            game->settings.username[game->settings.nameLength] = '\0';
        }
    }
    
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE)) {
        if (game->settings.nameLength == 0) {
            strcpy(game->settings.username, "Player");
            game->settings.nameLength = 6;
        }
        game->state = STATE_MENU;
    }
}

// ==================== 碰撞检测 ====================
void CheckCollisions(Game *game) {
    // 球与板子碰撞
    Rectangle paddleRect = {
        game->paddle.position.x,
        game->paddle.position.y,
        game->paddle.width,
        game->paddle.height
    };
    
    if (CheckCollisionCircleRec(game->ball.position, game->ball.radius, paddleRect)) {
        // 计算碰撞点相对于板子中心的位置，调整反弹角度
        float hitPoint = (game->ball.position.x - game->paddle.position.x) / game->paddle.width;
        float angle = (hitPoint - 0.5f) * 2.0f; // -1 到 1
        
        float ballSpeed = difficultyTable[game->settings.difficulty].ballSpeed;
        game->ball.speed.x = angle * ballSpeed;
        game->ball.speed.y = -fabsf(game->ball.speed.y);
        
        // 确保球不会卡在板子里
        game->ball.position.y = game->paddle.position.y - game->ball.radius - 1;
    }
    
    // 球与砖块碰撞
    for (int row = 0; row < BRICK_ROWS; row++) {
        for (int col = 0; col < BRICK_COLS; col++) {
            if (game->bricks[row][col].active) {
                if (CheckCollisionCircleRec(game->ball.position, game->ball.radius, game->bricks[row][col].rect)) {
                    game->bricks[row][col].active = false;
                    game->score += 1;
                    
                    // 确定碰撞方向
                    Vector2 ballCenter = game->ball.position;
                    Rectangle brickRect = game->bricks[row][col].rect;
                    
                    float brickCenterX = brickRect.x + brickRect.width / 2.0f;
                    float brickCenterY = brickRect.y + brickRect.height / 2.0f;
                    
                    float dx = ballCenter.x - brickCenterX;
                    float dy = ballCenter.y - brickCenterY;
                    
                    float overlapX = (brickRect.width / 2.0f + game->ball.radius) - fabsf(dx);
                    float overlapY = (brickRect.height / 2.0f + game->ball.radius) - fabsf(dy);
                    
                    if (overlapX < overlapY) {
                        // 水平碰撞
                        game->ball.speed.x *= -1;
                    } else {
                        // 垂直碰撞
                        game->ball.speed.y *= -1;
                    }
                    
                    return; // 每帧只处理一次碰撞
                }
            }
        }
    }
}

// ==================== 绘制函数 ====================
void DrawGame(Game *game) {
    BeginDrawing();
    ClearBackground((Color){ 20, 25, 35, 255 });
    
    switch (game->state) {
        case STATE_MENU:
            DrawMenu(game);
            break;
        case STATE_SETTINGS:
            DrawSettings(game);
            break;
        case STATE_PLAYING:
            DrawPlaying(game);
            break;
        case STATE_PAUSED:
            DrawPlaying(game);
            DrawPauseMenu(game);
            break;
        case STATE_QUIT_CONFIRM:
            DrawPlaying(game);
            DrawQuitConfirm(game);
            break;
        case STATE_FINAL_CONFIRM:
            DrawMenu(game);
            DrawFinalConfirm(game);
            break;
        case STATE_GAME_OVER:
            DrawPlaying(game);
            DrawGameOver(game);
            break;
        case STATE_ENTER_NAME:
            DrawEnterName(game);
            break;
    }
    
    EndDrawing();
}

void DrawBlurredRect(Rectangle rect, int blurStrength, Color tint) {
    // 简化的模糊效果 - 通过绘制多个半透明层实现
    Color blurColor = (Color){ tint.r, tint.g, tint.b, tint.a / 3 };
    
    for (int i = 0; i < blurStrength; i++) {
        DrawRectangleRec(rect, blurColor);
    }
    
    // 添加边框效果
    DrawRectangleLinesEx(rect, 2, (Color){ tint.r, tint.g, tint.b, tint.a / 2 });
}

void DrawMenu(Game *game) {
    // 绘制模糊背景
    DrawBlurredRect((Rectangle){ 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT }, 3, (Color){ 15, 20, 30, 200 });
    
    // 绘制装饰砖块背景
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 12; j++) {
            if ((i + j) % 3 != 0) {
                Color brickColor = GetRandomBrickColor(game->randomSeed + i * 100 + j, i, j);
                brickColor.a = 40;
                DrawRectangle(10 + j * 67, 10 + i * 35, 62, 30, brickColor);
            }
        }
    }
    
    // 标题
    DrawText("BREAKOUT", SCREEN_WIDTH/2 - MeasureText("BREAKOUT", 60)/2, 100, 60, (Color){ 255, 200, 100, 255 });
    DrawText("ENHANCED EDITION", SCREEN_WIDTH/2 - MeasureText("ENHANCED EDITION", 25)/2, 165, 25, (Color){ 200, 200, 200, 255 });
    
    // 用户名显示
    char welcomeText[64];
    sprintf(welcomeText, "Welcome, %s!", game->settings.username);
    DrawText(welcomeText, SCREEN_WIDTH/2 - MeasureText(welcomeText, 20)/2, 210, 20, (Color){ 150, 200, 255, 255 });
    
    // 菜单选项
    const char *menuItems[] = {
        "START GAME",
        NULL,  // 难度显示，特殊处理
        "SETTINGS",
        "CHANGE NAME",
        "QUIT"
    };
    
    int yPos = 280;
    for (int i = 0; i < 5; i++) {
        Color itemColor = (i == game->selectedMenuItem) ? (Color){ 255, 255, 100, 255 } : (Color){ 200, 200, 200, 255 };
        
        if (i == 1) {
            // 难度选择特殊显示
            char diffText[50];
            sprintf(diffText, "DIFFICULTY: %s", GetDifficultyName(game->settings.difficulty));
            int textWidth = MeasureText(diffText, 25);
            
            if (i == game->selectedMenuItem) {
                DrawRectangle(SCREEN_WIDTH/2 - textWidth/2 - 15, yPos - 5, textWidth + 30, 35, (Color){ 50, 60, 80, 200 });
                DrawRectangleLines(SCREEN_WIDTH/2 - textWidth/2 - 15, yPos - 5, textWidth + 30, 35, (Color){ 255, 255, 100, 255 });
            }
            DrawText(diffText, SCREEN_WIDTH/2 - textWidth/2, yPos, 25, itemColor);
            
            // 左右箭头提示
            DrawText("<", SCREEN_WIDTH/2 - textWidth/2 - 40, yPos, 25, (Color){ 150, 150, 150, 255 });
            DrawText(">", SCREEN_WIDTH/2 + textWidth/2 + 15, yPos, 25, (Color){ 150, 150, 150, 255 });
        } else {
            const char *text = menuItems[i];
            int textWidth = MeasureText(text, 25);
            
            if (i == game->selectedMenuItem) {
                DrawRectangle(SCREEN_WIDTH/2 - textWidth/2 - 15, yPos - 5, textWidth + 30, 35, (Color){ 50, 60, 80, 200 });
                DrawRectangleLines(SCREEN_WIDTH/2 - textWidth/2 - 15, yPos - 5, textWidth + 30, 35, (Color){ 255, 255, 100, 255 });
            }
            DrawText(text, SCREEN_WIDTH/2 - textWidth/2, yPos, 25, itemColor);
        }
        
        yPos += 50;
    }
    
    // 操作提示
    DrawText("Use UP/DOWN or mouse to select, ENTER to confirm", 
             SCREEN_WIDTH/2 - MeasureText("Use UP/DOWN or mouse to select, ENTER to confirm", 16)/2, 
             SCREEN_HEIGHT - 40, 16, (Color){ 120, 120, 120, 255 });
}

void DrawSettings(Game *game) {
    // 模糊背景
    DrawBlurredRect((Rectangle){ 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT }, 5, (Color){ 15, 20, 30, 220 });
    
    // 标题
    DrawText("SETTINGS", SCREEN_WIDTH/2 - MeasureText("SETTINGS", 40)/2, 80, 40, (Color){ 255, 200, 100, 255 });
    
    // 设置项
    char settingText[100];
    int yPos = 250;
    
    // 球颜色
    Color itemColor = (0 == game->selectedSettingsItem) ? (Color){ 255, 255, 100, 255 } : (Color){ 200, 200, 200, 255 };
    sprintf(settingText, "BALL COLOR");
    DrawText(settingText, SCREEN_WIDTH/2 - 150, yPos, 22, itemColor);
    DrawCircle(SCREEN_WIDTH/2 + 100, yPos + 10, 12, game->settings.ballColor);
    DrawText("< >", SCREEN_WIDTH/2 + 60, yPos, 22, (Color){ 150, 150, 150, 255 });
    
    yPos += 60;
    
    // 音频开关
    itemColor = (1 == game->selectedSettingsItem) ? (Color){ 255, 255, 100, 255 } : (Color){ 200, 200, 200, 255 };
    DrawText("SOUND", SCREEN_WIDTH/2 - 150, yPos, 22, itemColor);
    const char *soundStatus = game->settings.soundEnabled ? "ON" : "OFF";
    DrawText(soundStatus, SCREEN_WIDTH/2 + 80, yPos, 22, game->settings.soundEnabled ? (Color){ 100, 255, 100, 255 } : (Color){ 255, 100, 100, 255 });
    
    yPos += 60;
    
    // 清晰度
    itemColor = (2 == game->selectedSettingsItem) ? (Color){ 255, 255, 100, 255 } : (Color){ 200, 200, 200, 255 };
    DrawText("RESOLUTION SCALE", SCREEN_WIDTH/2 - 150, yPos, 22, itemColor);
    sprintf(settingText, "%.2fx", game->settings.resolutionScale);
    DrawText(settingText, SCREEN_WIDTH/2 + 80, yPos, 22, (Color){ 150, 200, 255, 255 });
    
    yPos += 60;
    
    // 难度显示（只读）
    itemColor = (3 == game->selectedSettingsItem) ? (Color){ 255, 255, 100, 255 } : (Color){ 200, 200, 200, 255 };
    DrawText("DIFFICULTY INFO", SCREEN_WIDTH/2 - 150, yPos, 22, itemColor);
    sprintf(settingText, "%s", GetDifficultyName(game->settings.difficulty));
    DrawText(settingText, SCREEN_WIDTH/2 + 80, yPos, 22, (Color){ 200, 150, 255, 255 });
    
    // 返回按钮
    Rectangle backBtn = { SCREEN_WIDTH/2 - 60, 490, 120, 40 };
    Color backColor = CheckCollisionPointRec(GetMousePosition(), backBtn) ? (Color){ 80, 100, 140, 255 } : (Color){ 50, 60, 80, 255 };
    DrawRectangleRec(backBtn, backColor);
    DrawText("BACK", SCREEN_WIDTH/2 - MeasureText("BACK", 22)/2, 500, 22, (Color){ 255, 255, 255, 255 });
    
    // 操作提示
    DrawText("UP/DOWN to select, LEFT/RIGHT to adjust, ESC to go back", 
             SCREEN_WIDTH/2 - MeasureText("UP/DOWN to select, LEFT/RIGHT to adjust, ESC to go back", 16)/2, 
             SCREEN_HEIGHT - 40, 16, (Color){ 120, 120, 120, 255 });
}

void DrawPlaying(Game *game) {
    // 绘制边框
    DrawRectangleLines(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){ 60, 70, 90, 255 });
    
    // 绘制砖块
    for (int row = 0; row < BRICK_ROWS; row++) {
        for (int col = 0; col < BRICK_COLS; col++) {
            if (game->bricks[row][col].active) {
                DrawRectangleRec(game->bricks[row][col].rect, game->bricks[row][col].color);
                DrawRectangleLinesEx(game->bricks[row][col].rect, 1, (Color){ 255, 255, 255, 100 });
            }
        }
    }
    
    // 绘制板子
    DrawRectangleRec((Rectangle){ 
        game->paddle.position.x, 
        game->paddle.position.y, 
        game->paddle.width, 
        game->paddle.height 
    }, game->paddle.color);
    DrawRectangleLinesEx((Rectangle){ 
        game->paddle.position.x, 
        game->paddle.position.y, 
        game->paddle.width, 
        game->paddle.height 
    }, 2, (Color){ 150, 200, 255, 255 });
    
    // 绘制球
    DrawCircleV(game->ball.position, game->ball.radius, game->ball.color);
    DrawCircleLines(game->ball.position.x, game->ball.position.y, game->ball.radius, (Color){ 255, 255, 255, 150 });
    
    // 绘制UI
    // 顶部信息栏背景
    DrawRectangle(0, 0, SCREEN_WIDTH, 35, (Color){ 30, 35, 45, 200 });
    
    char infoText[150];
    sprintf(infoText, "Player: %s  |  Score: %d  |  Lives: %d  |  Level: %d  |  %s", 
            game->settings.username, game->score, game->lives, game->level, 
            GetDifficultyName(game->settings.difficulty));
    DrawText(infoText, 10, 10, 18, (Color){ 200, 200, 200, 255 });
    
    // 操作提示
    if (!game->ball.active && game->gameStarted) {
        DrawText("Press SPACE or click to launch the ball", 
                 SCREEN_WIDTH/2 - MeasureText("Press SPACE or click to launch the ball", 18)/2, 
                 SCREEN_HEIGHT - 30, 18, (Color){ 150, 150, 150, 255 });
    } else if (!game->gameStarted) {
        DrawText("Press SPACE or click to start", 
                 SCREEN_WIDTH/2 - MeasureText("Press SPACE or click to start", 18)/2, 
                 SCREEN_HEIGHT - 30, 18, (Color){ 150, 150, 150, 255 });
    }
    
    DrawText("ESC: Pause", SCREEN_WIDTH - 100, SCREEN_HEIGHT - 25, 16, (Color){ 100, 100, 100, 255 });
}

void DrawPauseMenu(Game *game) {
    // 半透明覆盖层
    DrawBlurredRect((Rectangle){ 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT }, 4, (Color){ 0, 0, 0, 180 });
    
    // 暂停框
    Rectangle pauseBox = { SCREEN_WIDTH/2 - 150, 200, 300, 220 };
    DrawRectangleRec(pauseBox, (Color){ 30, 35, 50, 240 });
    DrawRectangleLinesEx(pauseBox, 3, (Color){ 100, 150, 255, 255 });
    
    DrawText("PAUSED", SCREEN_WIDTH/2 - MeasureText("PAUSED", 35)/2, 220, 35, (Color){ 255, 200, 100, 255 });
    
    // 按钮
    Rectangle continueBtn = { SCREEN_WIDTH/2 - 80, 280, 160, 45 };
    Rectangle quitBtn = { SCREEN_WIDTH/2 - 80, 340, 160, 45 };
    
    Color continueColor = CheckCollisionPointRec(GetMousePosition(), continueBtn) ? 
                          (Color){ 80, 130, 80, 255 } : (Color){ 50, 80, 50, 255 };
    Color quitColor = CheckCollisionPointRec(GetMousePosition(), quitBtn) ? 
                      (Color){ 130, 80, 80, 255 } : (Color){ 80, 50, 50, 255 };
    
    DrawRectangleRec(continueBtn, continueColor);
    DrawText("CONTINUE", SCREEN_WIDTH/2 - MeasureText("CONTINUE", 22)/2, 292, 22, (Color){ 255, 255, 255, 255 });
    
    DrawRectangleRec(quitBtn, quitColor);
    DrawText("QUIT", SCREEN_WIDTH/2 - MeasureText("QUIT", 22)/2, 352, 22, (Color){ 255, 255, 255, 255 });
}

void DrawQuitConfirm(Game *game) {
    // 更深的半透明覆盖
    DrawBlurredRect((Rectangle){ 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT }, 6, (Color){ 0, 0, 0, 200 });
    
    // 确认框
    Rectangle confirmBox = { SCREEN_WIDTH/2 - 200, 220, 400, 180 };
    DrawRectangleRec(confirmBox, (Color){ 40, 30, 30, 250 });
    DrawRectangleLinesEx(confirmBox, 3, (Color){ 255, 100, 100, 255 });
    
    DrawText("LEAVING WILL LOSE ALL GAME PROGRESS!", 
             SCREEN_WIDTH/2 - MeasureText("LEAVING WILL LOSE ALL GAME PROGRESS!", 18)/2, 250, 18, (Color){ 255, 200, 150, 255 });
    DrawText("Are you sure you want to quit?", 
             SCREEN_WIDTH/2 - MeasureText("Are you sure you want to quit?", 20)/2, 280, 20, (Color){ 255, 255, 255, 255 });
    
    // 按钮
    Rectangle yesBtn = { SCREEN_WIDTH/2 - 120, 320, 100, 45 };
    Rectangle noBtn = { SCREEN_WIDTH/2 + 20, 320, 100, 45 };
    
    Color yesColor = CheckCollisionPointRec(GetMousePosition(), yesBtn) ? 
                     (Color){ 150, 50, 50, 255 } : (Color){ 100, 40, 40, 255 };
    Color noColor = CheckCollisionPointRec(GetMousePosition(), noBtn) ? 
                    (Color){ 50, 100, 50, 255 } : (Color){ 40, 70, 40, 255 };
    
    DrawRectangleRec(yesBtn, yesColor);
    DrawText("YES", SCREEN_WIDTH/2 - 95, 332, 22, (Color){ 255, 255, 255, 255 });
    
    DrawRectangleRec(noBtn, noColor);
    DrawText("NO", SCREEN_WIDTH/2 + 50, 332, 22, (Color){ 255, 255, 255, 255 });
}

void DrawFinalConfirm(Game *game) {
    // 更深的半透明覆盖
    DrawBlurredRect((Rectangle){ 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT }, 6, (Color){ 0, 0, 0, 220 });
    
    // 确认框
    Rectangle confirmBox = { SCREEN_WIDTH/2 - 200, 220, 400, 180 };
    DrawRectangleRec(confirmBox, (Color){ 30, 30, 40, 250 });
    DrawRectangleLinesEx(confirmBox, 3, (Color){ 255, 200, 100, 255 });
    
    DrawText("QUIT GAME", 
             SCREEN_WIDTH/2 - MeasureText("QUIT GAME", 30)/2, 240, 30, (Color){ 255, 200, 100, 255 });
    DrawText("Are you sure you want to exit?", 
             SCREEN_WIDTH/2 - MeasureText("Are you sure you want to exit?", 20)/2, 280, 20, (Color){ 255, 255, 255, 255 });
    
    // 按钮
    Rectangle yesBtn = { SCREEN_WIDTH/2 - 120, 320, 100, 45 };
    Rectangle noBtn = { SCREEN_WIDTH/2 + 20, 320, 100, 45 };
    
    Color yesColor = CheckCollisionPointRec(GetMousePosition(), yesBtn) ? 
                     (Color){ 150, 50, 50, 255 } : (Color){ 100, 40, 40, 255 };
    Color noColor = CheckCollisionPointRec(GetMousePosition(), noBtn) ? 
                    (Color){ 50, 100, 50, 255 } : (Color){ 40, 70, 40, 255 };
    
    DrawRectangleRec(yesBtn, yesColor);
    DrawText("YES", SCREEN_WIDTH/2 - 95, 332, 22, (Color){ 255, 255, 255, 255 });
    
    DrawRectangleRec(noBtn, noColor);
    DrawText("NO", SCREEN_WIDTH/2 + 50, 332, 22, (Color){ 255, 255, 255, 255 });
}

void DrawGameOver(Game *game) {
    // 半透明覆盖
    DrawBlurredRect((Rectangle){ 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT }, 5, (Color){ 0, 0, 0, 200 });
    
    // 游戏结束框
    Rectangle gameOverBox = { SCREEN_WIDTH/2 - 180, 150, 360, 360 };
    DrawRectangleRec(gameOverBox, (Color){ 25, 30, 45, 245 });
    DrawRectangleLinesEx(gameOverBox, 3, (Color){ 255, 200, 100, 255 });
    
    // 标题和评级
    const char *rating = GetScoreRating(game->score);
    Color ratingColor;
    if (game->score < 10) {
        ratingColor = (Color){ 200, 200, 200, 255 };
    } else if (game->score < 20) {
        ratingColor = (Color){ 100, 255, 150, 255 };
    } else {
        ratingColor = (Color){ 255, 215, 0, 255 };
    }
    
    DrawText("GAME OVER", SCREEN_WIDTH/2 - MeasureText("GAME OVER", 35)/2, 170, 35, (Color){ 255, 100, 100, 255 });
    
    // 大号评级显示
    int ratingWidth = MeasureText(rating, 50);
    DrawText(rating, SCREEN_WIDTH/2 - ratingWidth/2, 220, 50, ratingColor);
    
    // 分数显示
    char scoreText[50];
    sprintf(scoreText, "Final Score: %d", game->score);
    DrawText(scoreText, SCREEN_WIDTH/2 - MeasureText(scoreText, 25)/2, 290, 25, (Color){ 255, 255, 255, 255 });
    
    // 关卡显示
    sprintf(scoreText, "Level Reached: %d", game->level);
    DrawText(scoreText, SCREEN_WIDTH/2 - MeasureText(scoreText, 22)/2, 325, 22, (Color){ 180, 180, 180, 255 });
    
    // 按钮
    Rectangle menuBtn = { SCREEN_WIDTH/2 - 100, 380, 200, 45 };
    Rectangle restartBtn = { SCREEN_WIDTH/2 - 100, 440, 200, 45 };
    
    Color menuColor = CheckCollisionPointRec(GetMousePosition(), menuBtn) ? 
                      (Color){ 80, 100, 140, 255 } : (Color){ 50, 60, 80, 255 };
    Color restartColor = CheckCollisionPointRec(GetMousePosition(), restartBtn) ? 
                         (Color){ 80, 130, 80, 255 } : (Color){ 50, 80, 50, 255 };
    
    DrawRectangleRec(menuBtn, menuColor);
    DrawText("MAIN MENU", SCREEN_WIDTH/2 - MeasureText("MAIN MENU", 22)/2, 392, 22, (Color){ 255, 255, 255, 255 });
    
    DrawRectangleRec(restartBtn, restartColor);
    DrawText("PLAY AGAIN", SCREEN_WIDTH/2 - MeasureText("PLAY AGAIN", 22)/2, 452, 22, (Color){ 255, 255, 255, 255 });
    
    // 提示
    DrawText("Press ENTER for menu, R to restart", 
             SCREEN_WIDTH/2 - MeasureText("Press ENTER for menu, R to restart", 16)/2, 
             SCREEN_HEIGHT - 50, 16, (Color){ 120, 120, 120, 255 });
}

void DrawEnterName(Game *game) {
    // 模糊背景
    DrawBlurredRect((Rectangle){ 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT }, 5, (Color){ 15, 20, 30, 220 });
    
    // 标题
    DrawText("ENTER YOUR NAME", SCREEN_WIDTH/2 - MeasureText("ENTER YOUR NAME", 35)/2, 180, 35, (Color){ 255, 200, 100, 255 });
    
    // 输入框
    Rectangle inputBox = { SCREEN_WIDTH/2 - 150, 260, 300, 50 };
    DrawRectangleRec(inputBox, (Color){ 40, 45, 60, 255 });
    DrawRectangleLinesEx(inputBox, 2, (Color){ 100, 150, 255, 255 });
    
    // 显示输入的名字
    DrawText(game->settings.username, inputBox.x + 10, inputBox.y + 15, 25, (Color){ 255, 255, 255, 255 });
    
    // 闪烁光标
    if (((int)(GetTime() * 2) % 2) == 0) {
        int cursorX = inputBox.x + 10 + MeasureText(game->settings.username, 25);
        DrawRectangle(cursorX, inputBox.y + 15, 2, 25, (Color){ 255, 255, 255, 255 });
    }
    
    // 字符计数
    char countText[30];
    sprintf(countText, "%d/%d chars", game->settings.nameLength, MAX_INPUT_CHARS);
    DrawText(countText, SCREEN_WIDTH/2 - MeasureText(countText, 16)/2, 320, 16, (Color){ 150, 150, 150, 255 });
    
    // 操作提示
    DrawText("Type your name and press ENTER when done", 
             SCREEN_WIDTH/2 - MeasureText("Type your name and press ENTER when done", 18)/2, 
             370, 18, (Color){ 120, 120, 120, 255 });
    DrawText("Press ESC to cancel and keep current name", 
             SCREEN_WIDTH/2 - MeasureText("Press ESC to cancel and keep current name", 16)/2, 
             400, 16, (Color){ 100, 100, 100, 255 });
}

// ==================== 工具函数 ====================
Color GetRandomBrickColor(unsigned int seed, int row, int col) {
    // 使用行和列以及种子生成伪随机颜色
    unsigned int hash = seed + row * 31 + col * 17;
    hash = ((hash >> 16) ^ hash) * 0x45d9f3b;
    hash = ((hash >> 16) ^ hash) * 0x45d9f3b;
    hash = (hash >> 16) ^ hash;
    
    // 生成鲜艳的颜色
    unsigned char hue = (unsigned char)(hash % 360);
    
    // HSV to RGB 转换（简化版）
    float h = hue / 60.0f;
    int i = (int)h % 6;
    float f = h - (int)h;
    float v = 0.9f;  // 明度
    float s = 0.8f;  // 饱和度
    
    float p = v * (1 - s);
    float q = v * (1 - f * s);
    float t = v * (1 - (1 - f) * s);
    
    Color color;
    switch (i) {
        case 0: color = (Color){ (unsigned char)(v*255), (unsigned char)(t*255), (unsigned char)(p*255), 255 }; break;
        case 1: color = (Color){ (unsigned char)(q*255), (unsigned char)(v*255), (unsigned char)(p*255), 255 }; break;
        case 2: color = (Color){ (unsigned char)(p*255), (unsigned char)(v*255), (unsigned char)(t*255), 255 }; break;
        case 3: color = (Color){ (unsigned char)(p*255), (unsigned char)(q*255), (unsigned char)(v*255), 255 }; break;
        case 4: color = (Color){ (unsigned char)(t*255), (unsigned char)(p*255), (unsigned char)(v*255), 255 }; break;
        case 5: color = (Color){ (unsigned char)(v*255), (unsigned char)(p*255), (unsigned char)(q*255), 255 }; break;
        default: color = (Color){ 200, 200, 200, 255 }; break;
    }
    
    return color;
}

const char* GetDifficultyName(Difficulty diff) {
    switch (diff) {
        case DIFFICULTY_EASY: return "EASY";
        case DIFFICULTY_MEDIUM: return "MEDIUM";
        case DIFFICULTY_HARD: return "HARD";
        default: return "UNKNOWN";
    }
}

const char* GetScoreRating(int score) {
    if (score < 10) {
        return "GOOD";
    } else if (score < 20) {
        return "GREAT";
    } else {
        return "BRILLIANT";
    }
}
