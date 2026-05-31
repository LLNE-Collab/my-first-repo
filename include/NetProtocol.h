#pragma once

/**
 * @file NetProtocol.h
 * @brief 局域网合作对战：TCP + 换行符分帧的 JSON 消息协议。
 *
 * 主机（Server）：运行完整物理，广播 state。
 * 客机（Client）：发送挡板 input，接收并渲染 state。
 */

#include <nlohmann/json.hpp>
#include <raylib.h>
#include <string>
#include <vector>

using json = nlohmann::json;

namespace net {

inline constexpr uint16_t kDefaultPort = 5555;

enum class MsgType {
    Hello,
    Start,
    Input,
    State,
    Bye,
    Unknown
};

inline MsgType ParseType(const json& j) {
    if (!j.contains("type") || !j["type"].is_string()) {
        return MsgType::Unknown;
    }
    const std::string t = j["type"].get<std::string>();
    if (t == "hello") return MsgType::Hello;
    if (t == "start") return MsgType::Start;
    if (t == "input") return MsgType::Input;
    if (t == "state") return MsgType::State;
    if (t == "bye") return MsgType::Bye;
    return MsgType::Unknown;
}

inline json MakeHello(const std::string& role) {
    return json{{"type", "hello"}, {"role", role}};
}

inline json MakeStart(int level) {
    return json{{"type", "start"}, {"level", level}};
}

inline json MakeInput(float x, float y) {
    return json{{"type", "input"}, {"x", x}, {"y", y}};
}

inline json MakeBye() {
    return json{{"type", "bye"}};
}

struct GameSnapshot {
    Vector2 ballPos{};
    Vector2 ballVel{};
    float ballRadius{10.0f};
    Vector2 paddle1{};
    Vector2 paddle2{};
    float paddleW{100.0f};
    float paddleH{10.0f};
    int score{0};
    int lives{0};
    int currentLevel{1};
    float ballRespawnTimer{0.0f};
    std::vector<uint8_t> brickActive;
};

inline json SerializeState(const GameSnapshot& s) {
    json bricks = json::array();
    for (uint8_t active : s.brickActive) {
        bricks.push_back(active != 0);
    }
    return json{
        {"type", "state"},
        {"ball", {{"x", s.ballPos.x}, {"y", s.ballPos.y}, {"vx", s.ballVel.x}, {"vy", s.ballVel.y}, {"r", s.ballRadius}}},
        {"p1", {{"x", s.paddle1.x}, {"y", s.paddle1.y}, {"w", s.paddleW}, {"h", s.paddleH}}},
        {"p2", {{"x", s.paddle2.x}, {"y", s.paddle2.y}, {"w", s.paddleW}, {"h", s.paddleH}}},
        {"score", s.score},
        {"lives", s.lives},
        {"level", s.currentLevel},
        {"respawn", s.ballRespawnTimer},
        {"bricks", bricks}
    };
}

inline bool DeserializeState(const json& j, GameSnapshot& out) {
    if (ParseType(j) != MsgType::State) {
        return false;
    }
    const json& ball = j.value("ball", json::object());
    out.ballPos = {ball.value("x", 0.0f), ball.value("y", 0.0f)};
    out.ballVel = {ball.value("vx", 0.0f), ball.value("vy", 0.0f)};
    out.ballRadius = ball.value("r", 10.0f);

    const json& p1 = j.value("p1", json::object());
    const json& p2 = j.value("p2", json::object());
    out.paddle1 = {p1.value("x", 0.0f), p1.value("y", 0.0f)};
    out.paddle2 = {p2.value("x", 0.0f), p2.value("y", 0.0f)};
    out.paddleW = p1.value("w", 100.0f);
    out.paddleH = p1.value("h", 10.0f);

    out.score = j.value("score", 0);
    out.lives = j.value("lives", 0);
    out.currentLevel = j.value("level", 1);
    out.ballRespawnTimer = j.value("respawn", 0.0f);

    out.brickActive.clear();
    if (j.contains("bricks") && j["bricks"].is_array()) {
        for (const auto& b : j["bricks"]) {
            out.brickActive.push_back(b.get<bool>() ? 1 : 0);
        }
    }
    return true;
}

}  // namespace net
