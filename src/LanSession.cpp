#include "LanSession.h"

#include <raylib.h>

#include <arpa/inet.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <vector>

namespace {

void CloseFd(int& fd) {
    if (fd >= 0) {
        close(fd);
        fd = -1;
    }
}

}  // namespace

LanSession::LanSession() = default;

LanSession::~LanSession() {
    Close();
}

bool LanSession::SetNonBlocking(int fd) {
    const int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        return false;
    }
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK) >= 0;
}

bool LanSession::StartHost(uint16_t port) {
    Close();

    listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd_ < 0) {
        statusMessage_ = "创建 socket 失败";
        return false;
    }

    int yes = 1;
    setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(listenFd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        statusMessage_ = "bind 失败，端口可能被占用";
        CloseFd(listenFd_);
        return false;
    }

    if (listen(listenFd_, 1) < 0) {
        statusMessage_ = "listen 失败";
        CloseFd(listenFd_);
        return false;
    }

    SetNonBlocking(listenFd_);
    isHost_ = true;
    listening_ = true;
    connected_ = false;
    statusMessage_ = "等待队友加入...";
    return true;
}

bool LanSession::Connect(const std::string& host, uint16_t port) {
    Close();

    peerFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (peerFd_ < 0) {
        statusMessage_ = "创建 socket 失败";
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0) {
        statusMessage_ = "IP 地址无效";
        CloseFd(peerFd_);
        return false;
    }

    if (connect(peerFd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        statusMessage_ = "连接失败，请确认主机已开房且在同一局域网";
        CloseFd(peerFd_);
        return false;
    }

    SetNonBlocking(peerFd_);
    isHost_ = false;
    listening_ = false;
    connected_ = true;
    statusMessage_ = "已连接主机";
    return true;
}

void LanSession::Close() {
    if (connected_ && peerFd_ >= 0) {
        SendRaw(R"({"type":"bye"})" "\n");
    }
    CloseFd(peerFd_);
    CloseFd(listenFd_);
    isHost_ = false;
    listening_ = false;
    connected_ = false;
    recvBuffer_.clear();
}

bool LanSession::SendRaw(const std::string& data) {
    if (peerFd_ < 0) {
        return false;
    }
    const char* buf = data.c_str();
    size_t total = 0;
    while (total < data.size()) {
        const ssize_t n = send(peerFd_, buf + total, data.size() - total, MSG_NOSIGNAL);
        if (n <= 0) {
            return false;
        }
        total += static_cast<size_t>(n);
    }
    return true;
}

bool LanSession::SendJsonLine(const std::string& line) {
    if (!connected_) {
        return false;
    }
    return SendRaw(line + "\n");
}

void LanSession::PumpRecv() {
    if (listening_ && !connected_ && listenFd_ >= 0) {
        sockaddr_in clientAddr{};
        socklen_t len = sizeof(clientAddr);
        const int clientFd = accept(listenFd_, reinterpret_cast<sockaddr*>(&clientAddr), &len);
        if (clientFd >= 0) {
            peerFd_ = clientFd;
            SetNonBlocking(peerFd_);
            connected_ = true;
            listening_ = false;
            CloseFd(listenFd_);
            char ip[INET_ADDRSTRLEN]{};
            inet_ntop(AF_INET, &clientAddr.sin_addr, ip, sizeof(ip));
            statusMessage_ = std::string("队友已连接: ") + ip;
            TraceLog(LOG_INFO, "LAN: client connected from %s", ip);
        }
    }

    if (peerFd_ < 0) {
        return;
    }

    char chunk[4096];
    while (true) {
        const ssize_t n = recv(peerFd_, chunk, sizeof(chunk), 0);
        if (n > 0) {
            recvBuffer_.append(chunk, static_cast<size_t>(n));
        } else if (n == 0) {
            statusMessage_ = "连接已断开";
            CloseFd(peerFd_);
            connected_ = false;
            break;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            statusMessage_ = "recv 错误，连接关闭";
            CloseFd(peerFd_);
            connected_ = false;
            break;
        }
    }
}

bool LanSession::TryPopJsonLine(std::string& outLine) {
    PumpRecv();
    const size_t pos = recvBuffer_.find('\n');
    if (pos == std::string::npos) {
        return false;
    }
    outLine = recvBuffer_.substr(0, pos);
    recvBuffer_.erase(0, pos + 1);
    return true;
}

std::string LanSession::GetLocalIpHint() {
    std::vector<std::string> ips;
    ifaddrs* ifaddr = nullptr;
    if (getifaddrs(&ifaddr) == 0) {
        for (ifaddrs* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
            if (!ifa->ifa_addr || ifa->ifa_addr->sa_family != AF_INET) {
                continue;
            }
            char host[INET_ADDRSTRLEN]{};
            const void* addr = &reinterpret_cast<sockaddr_in*>(ifa->ifa_addr)->sin_addr;
            inet_ntop(AF_INET, addr, host, sizeof(host));
            const std::string ip(host);
            if (ip != "127.0.0.1") {
                ips.push_back(ip);
            }
        }
        freeifaddrs(ifaddr);
    }
    if (ips.empty()) {
        return "127.0.0.1 (本机测试)";
    }
    std::string result = ips.front();
    for (size_t i = 1; i < ips.size(); ++i) {
        result += " / " + ips[i];
    }
    return result;
}
