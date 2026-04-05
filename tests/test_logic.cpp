#include <gtest/gtest.h>
#include "raylib.h" // 需要 Raylib 的结构定义，但不需要图形窗口

// 模拟游戏中的碰撞检测逻辑进行测试
bool CheckCollisionSimple(Vector2 ballPos, float radius, Rectangle rec) {
    // 简化的圆与矩形碰撞检测
    float closestX = ballPos.x;
    float closestY = ballPos.y;
    
    if (ballPos.x < rec.x) closestX = rec.x;
    else if (ballPos.x > rec.x + rec.width) closestX = rec.x + rec.width;
    
    if (ballPos.y < rec.y) closestY = rec.y;
    else if (ballPos.y > rec.y + rec.height) closestY = rec.y + rec.height;
    
    float dx = ballPos.x - closestX;
    float dy = ballPos.y - closestY;
    
    return (dx * dx + dy * dy) < (radius * radius);
}

TEST(CollisionTest, BallInsideRect) {
    Vector2 pos = {100, 100};
    Rectangle rec = {50, 50, 100, 100};
    EXPECT_TRUE(CheckCollisionSimple(pos, 10, rec));
}

TEST(CollisionTest, BallOutsideRect) {
    Vector2 pos = {0, 0};
    Rectangle rec = {50, 50, 100, 100};
    EXPECT_FALSE(CheckCollisionSimple(pos, 10, rec));
}
