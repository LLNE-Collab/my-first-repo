#include "raylib.h"
#include <stdlib.h>
#include <time.h>

// 定义球的结构体
typedef struct Ball {
    Vector2 position;
    Vector2 velocity;
    float radius;
} Ball;

// 定义板的结构体
typedef struct Paddle {
    Vector2 position;
    float width;
    float height;
} Paddle;

// 定义砖块的结构体
typedef struct Brick {
    Rectangle rect;
    bool active;
    Color color;
} Brick;

// 初始化游戏对象
void InitGame(Ball *ball, Paddle *paddle, Brick bricks[], int brickCount) {
    // 初始化球
    ball->position = (Vector2){400, 300};
    ball->velocity = (Vector2){4, -4};
    ball->radius = 10;

    // 初始化板
    paddle->position = (Vector2){350, 550};
    paddle->width = 100;
    paddle->height = 10;

    // 初始化砖块（5行10列）
    int cols = 10;
    int brickWidth = 75, brickHeight = 20, spacing = 5;
    int startX = 25, startY = 50;

    for (int i = 0; i < brickCount; i++) {
        int row = i / cols, col = i % cols;
        bricks[i].rect = (Rectangle){startX + col * (brickWidth + spacing), 
                                   startY + row * (brickHeight + spacing), 
                                   brickWidth, brickHeight};
        bricks[i].active = true;
        bricks[i].color = (Color){rand() % 256, rand() % 256, rand() % 256, 255};
    }
}

// 更新球的位置和边界碰撞
bool UpdateBall(Ball *ball) {
    ball->position.x += ball->velocity.x;
    ball->position.y += ball->velocity.y;

    // 左右边界碰撞
    if (ball->position.x - ball->radius <= 0 || ball->position.x + ball->radius >= 800) {
        ball->velocity.x *= -1;
    }
    // 上边界碰撞
    if (ball->position.y - ball->radius <= 0) {
        ball->velocity.y *= -1;
    }
    // 球掉出底部，返回true
    if (ball->position.y + ball->radius >= 600) {
        return true;
    }
    return false;
}

// 更新板的位置（键盘控制）
void UpdatePaddle(Paddle *paddle) {
    if (IsKeyDown(KEY_LEFT) && paddle->position.x > 0) {
        paddle->position.x -= 7;
    }
    if (IsKeyDown(KEY_RIGHT) && paddle->position.x + paddle->width < 800) {
        paddle->position.x += 7;
    }
}

// 检测球与板的碰撞
void CheckPaddleCollision(Ball *ball, Paddle *paddle) {
    if (CheckCollisionCircleRec(ball->position, ball->radius, 
                              (Rectangle){paddle->position.x, paddle->position.y, 
                                         paddle->width, paddle->height})) {
        ball->velocity.y *= -1;
        // 根据碰撞位置调整水平速度（可选）
        float hitPos = (ball->position.x - paddle->position.x) / paddle->width;
        ball->velocity.x = 8 * (hitPos - 0.5f);
    }
}

// 检测球与砖的碰撞
void CheckBrickCollision(Ball *ball, Brick bricks[], int brickCount, int *score) {
    for (int i = 0; i < brickCount; i++) {
        if (bricks[i].active && CheckCollisionCircleRec(ball->position, ball->radius, bricks[i].rect)) {
            bricks[i].active = false;
            ball->velocity.y *= -1;
            (*score) += 10; // 每块砖10分
            break; // 一次只处理一个砖块
        }
    }
}

// 绘制游戏对象
void DrawGame(Ball ball, Paddle paddle, Brick bricks[], int brickCount, int lives, int score) {
    BeginDrawing();
    ClearBackground(RAYWHITE);

    // 绘制球
    DrawCircleV(ball.position, ball.radius, MAROON);

    // 绘制板
    DrawRectangleV(paddle.position, (Vector2){paddle.width, paddle.height}, DARKBLUE);

    // 绘制砖块
    for (int i = 0; i < brickCount; i++) {
        if (bricks[i].active) {
            DrawRectangleRec(bricks[i].rect, bricks[i].color);
        }
    }

    // 显示得分和生命
    DrawText(TextFormat("Score: %d", score), 10, 10, 20, BLACK);
    DrawText(TextFormat("Lives: %d", lives), 10, 40, 20, BLACK);

    EndDrawing();
}

int main() {
    const int screenWidth = 800;
    const int screenHeight = 600;
    InitWindow(screenWidth, screenHeight, "Breakout Game");

    Ball ball;
    Paddle paddle;
    const int brickCount = 50; // 5行10列
    Brick bricks[brickCount];
    int lives = 3;
    int score = 0;
    bool gameOver = false;
    bool win = false;

    InitGame(&ball, &paddle, bricks, brickCount);
    srand(time(NULL)); // 初始化随机种子
    SetTargetFPS(60);

    while (!WindowShouldClose() && !gameOver && !win) {
        if (UpdateBall(&ball)) {
            lives--;
            if (lives <= 0) {
                gameOver = true;
            } else {
                // 重置球位置
                ball.position = (Vector2){400, 300};
                ball.velocity = (Vector2){4, -4};
            }
        }
        UpdatePaddle(&paddle);
        CheckPaddleCollision(&ball, &paddle);
        CheckBrickCollision(&ball, bricks, brickCount, &score);

        // 检查胜利
        win = true;
        for (int i = 0; i < brickCount; i++) {
            if (bricks[i].active) {
                win = false;
                break;
            }
        }

        DrawGame(ball, paddle, bricks, brickCount, lives, score);
    }

    // 显示游戏结束或胜利
    BeginDrawing();
    ClearBackground(RAYWHITE);
    if (gameOver) {
        DrawText("Game Over!", 300, 250, 40, RED);
        DrawText(TextFormat("Final Score: %d", score), 300, 300, 30, BLACK);
    } else if (win) {
        DrawText("You Win!", 300, 250, 40, GREEN);
        DrawText(TextFormat("Score: %d", score), 300, 300, 30, BLACK);
    }
    EndDrawing();
    WaitTime(3.0f); // 显示3秒

    CloseWindow();
    return 0;
}
