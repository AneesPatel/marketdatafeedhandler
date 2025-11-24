#pragma once

#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <functional>
#include <cstdint>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
using socket_t = SOCKET;
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
using socket_t = int;
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close
#endif

struct WebSocketClient {
    socket_t socket;
    bool active;
    
    WebSocketClient() : socket(INVALID_SOCKET), active(false) {}
};

class WebSocketServer {
    socket_t server_socket_;
    uint16_t port_;
    std::atomic<bool> running_;
    std::thread accept_thread_;
    std::vector<WebSocketClient> clients_;
    
    void accept_loop();
    bool handle_handshake(socket_t client_socket);
    std::string generate_accept_key(const std::string& key);
    
public:
    explicit WebSocketServer(uint16_t port);
    ~WebSocketServer();
    
    void start();
    void stop();
    
    void broadcast(const std::string& message);
    void broadcast_binary(const uint8_t* data, size_t size);
    
    size_t client_count() const;
};
