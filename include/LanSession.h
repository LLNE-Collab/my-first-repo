#pragma once

/**
 * @file LanSession.h
 * @brief 局域网 TCP 会话：主机 listen/accept 或客机 connect，换行符分帧收发 JSON。
 */

#include <cstdint>
#include <string>

class LanSession {
public:
    LanSession();
    ~LanSession();

    LanSession(const LanSession&) = delete;
    LanSession& operator=(const LanSession&) = delete;

    bool StartHost(uint16_t port);
    bool Connect(const std::string& host, uint16_t port);
    void Close();

    [[nodiscard]] bool IsHost() const { return isHost_; }
    [[nodiscard]] bool IsConnected() const { return connected_; }
    [[nodiscard]] bool IsListening() const { return listening_; }

    bool SendJsonLine(const std::string& line);
    bool TryPopJsonLine(std::string& outLine);

    [[nodiscard]] std::string GetStatusMessage() const { return statusMessage_; }
    [[nodiscard]] static std::string GetLocalIpHint();

private:
    bool SetNonBlocking(int fd);
    void PumpRecv();
    bool SendRaw(const std::string& data);

    int listenFd_{-1};
    int peerFd_{-1};
    bool isHost_{false};
    bool listening_{false};
    bool connected_{false};
    std::string recvBuffer_;
    std::string statusMessage_;
};
