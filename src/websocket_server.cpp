#include "websocket_server.hpp"
#include <cstring>
#include <sstream>
#include <iomanip>

#ifdef _WIN32
#include <wincrypt.h>
#else
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#endif

static std::string base64_encode(const unsigned char* buffer, size_t length) {
    static const char* base64_chars = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
    std::string result;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];
    
    while (length--) {
        char_array_3[i++] = *(buffer++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;
            
            for (i = 0; i < 4; i++) {
                result += base64_chars[char_array_4[i]];
            }
            i = 0;
        }
    }
    
    if (i) {
        for (j = i; j < 3; j++) {
            char_array_3[j] = '\0';
        }
        
        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        
        for (j = 0; j < i + 1; j++) {
            result += base64_chars[char_array_4[j]];
        }
        
        while (i++ < 3) {
            result += '=';
        }
    }
    
    return result;
}

static void simple_sha1(const std::string& input, unsigned char* output) {
    uint32_t h0 = 0x67452301;
    uint32_t h1 = 0xEFCDAB89;
    uint32_t h2 = 0x98BADCFE;
    uint32_t h3 = 0x10325476;
    uint32_t h4 = 0xC3D2E1F0;
    
    std::vector<uint8_t> padded;
    for (char c : input) padded.push_back(c);
    
    uint64_t bit_len = input.size() * 8;
    padded.push_back(0x80);
    
    while ((padded.size() % 64) != 56) {
        padded.push_back(0);
    }
    
    for (int i = 7; i >= 0; i--) {
        padded.push_back((bit_len >> (i * 8)) & 0xFF);
    }
    
    auto rotate_left = [](uint32_t value, int bits) {
        return (value << bits) | (value >> (32 - bits));
    };
    
    for (size_t chunk = 0; chunk < padded.size(); chunk += 64) {
        uint32_t w[80];
        
        for (int i = 0; i < 16; i++) {
            w[i] = (padded[chunk + i*4] << 24) |
                   (padded[chunk + i*4 + 1] << 16) |
                   (padded[chunk + i*4 + 2] << 8) |
                   (padded[chunk + i*4 + 3]);
        }
        
        for (int i = 16; i < 80; i++) {
            w[i] = rotate_left(w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16], 1);
        }
        
        uint32_t a = h0, b = h1, c = h2, d = h3, e = h4;
        
        for (int i = 0; i < 80; i++) {
            uint32_t f, k;
            
            if (i < 20) {
                f = (b & c) | ((~b) & d);
                k = 0x5A827999;
            } else if (i < 40) {
                f = b ^ c ^ d;
                k = 0x6ED9EBA1;
            } else if (i < 60) {
                f = (b & c) | (b & d) | (c & d);
                k = 0x8F1BBCDC;
            } else {
                f = b ^ c ^ d;
                k = 0xCA62C1D6;
            }
            
            uint32_t temp = rotate_left(a, 5) + f + e + k + w[i];
            e = d;
            d = c;
            c = rotate_left(b, 30);
            b = a;
            a = temp;
        }
        
        h0 += a;
        h1 += b;
        h2 += c;
        h3 += d;
        h4 += e;
    }
    
    output[0] = (h0 >> 24) & 0xFF;
    output[1] = (h0 >> 16) & 0xFF;
    output[2] = (h0 >> 8) & 0xFF;
    output[3] = h0 & 0xFF;
    output[4] = (h1 >> 24) & 0xFF;
    output[5] = (h1 >> 16) & 0xFF;
    output[6] = (h1 >> 8) & 0xFF;
    output[7] = h1 & 0xFF;
    output[8] = (h2 >> 24) & 0xFF;
    output[9] = (h2 >> 16) & 0xFF;
    output[10] = (h2 >> 8) & 0xFF;
    output[11] = h2 & 0xFF;
    output[12] = (h3 >> 24) & 0xFF;
    output[13] = (h3 >> 16) & 0xFF;
    output[14] = (h3 >> 8) & 0xFF;
    output[15] = h3 & 0xFF;
    output[16] = (h4 >> 24) & 0xFF;
    output[17] = (h4 >> 16) & 0xFF;
    output[18] = (h4 >> 8) & 0xFF;
    output[19] = h4 & 0xFF;
}

WebSocketServer::WebSocketServer(uint16_t port)
    : server_socket_(INVALID_SOCKET)
    , port_(port)
    , running_(false) {
    
#ifdef _WIN32
    WSADATA wsa_data;
    WSAStartup(MAKEWORD(2, 2), &wsa_data);
#endif
}

WebSocketServer::~WebSocketServer() {
    stop();
    
#ifdef _WIN32
    WSACleanup();
#endif
}

void WebSocketServer::start() {
    server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_ == INVALID_SOCKET) {
        return;
    }
    
    int opt = 1;
    setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, 
               reinterpret_cast<const char*>(&opt), sizeof(opt));
    
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);
    
    if (bind(server_socket_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        closesocket(server_socket_);
        server_socket_ = INVALID_SOCKET;
        return;
    }
    
    if (listen(server_socket_, 10) == SOCKET_ERROR) {
        closesocket(server_socket_);
        server_socket_ = INVALID_SOCKET;
        return;
    }
    
    running_ = true;
    accept_thread_ = std::thread(&WebSocketServer::accept_loop, this);
}

void WebSocketServer::stop() {
    running_ = false;
    
    if (server_socket_ != INVALID_SOCKET) {
        closesocket(server_socket_);
        server_socket_ = INVALID_SOCKET;
    }
    
    if (accept_thread_.joinable()) {
        accept_thread_.join();
    }
    
    for (auto& client : clients_) {
        if (client.socket != INVALID_SOCKET) {
            closesocket(client.socket);
        }
    }
    clients_.clear();
}

void WebSocketServer::accept_loop() {
    while (running_) {
        sockaddr_in client_addr{};
        socklen_t addr_len = sizeof(client_addr);
        
        socket_t client_socket = accept(server_socket_, 
            reinterpret_cast<sockaddr*>(&client_addr), &addr_len);
        
        if (client_socket == INVALID_SOCKET) {
            continue;
        }
        
        if (handle_handshake(client_socket)) {
            WebSocketClient client;
            client.socket = client_socket;
            client.active = true;
            clients_.push_back(client);
        } else {
            closesocket(client_socket);
        }
    }
}

bool WebSocketServer::handle_handshake(socket_t client_socket) {
    char buffer[4096];
    int received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    
    if (received <= 0) {
        return false;
    }
    
    buffer[received] = '\0';
    std::string request(buffer);
    
    size_t key_pos = request.find("Sec-WebSocket-Key:");
    if (key_pos == std::string::npos) {
        return false;
    }
    
    key_pos += 18;
    while (key_pos < request.size() && request[key_pos] == ' ') key_pos++;
    
    size_t key_end = request.find("\r\n", key_pos);
    if (key_end == std::string::npos) {
        return false;
    }
    
    std::string key = request.substr(key_pos, key_end - key_pos);
    std::string accept = generate_accept_key(key);
    
    std::ostringstream response;
    response << "HTTP/1.1 101 Switching Protocols\r\n";
    response << "Upgrade: websocket\r\n";
    response << "Connection: Upgrade\r\n";
    response << "Sec-WebSocket-Accept: " << accept << "\r\n";
    response << "\r\n";
    
    std::string response_str = response.str();
    send(client_socket, response_str.c_str(), response_str.size(), 0);
    
    return true;
}

std::string WebSocketServer::generate_accept_key(const std::string& key) {
    const std::string magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    std::string combined = key + magic;
    
    unsigned char hash[20];
    simple_sha1(combined, hash);
    
    return base64_encode(hash, 20);
}

void WebSocketServer::broadcast(const std::string& message) {
    std::vector<uint8_t> frame;
    frame.push_back(0x81);
    
    size_t len = message.size();
    if (len <= 125) {
        frame.push_back(static_cast<uint8_t>(len));
    } else if (len <= 65535) {
        frame.push_back(126);
        frame.push_back((len >> 8) & 0xFF);
        frame.push_back(len & 0xFF);
    } else {
        frame.push_back(127);
        for (int i = 7; i >= 0; i--) {
            frame.push_back((len >> (i * 8)) & 0xFF);
        }
    }
    
    for (char c : message) {
        frame.push_back(static_cast<uint8_t>(c));
    }
    
    for (auto& client : clients_) {
        if (client.active && client.socket != INVALID_SOCKET) {
            send(client.socket, reinterpret_cast<const char*>(frame.data()), 
                 frame.size(), 0);
        }
    }
}

void WebSocketServer::broadcast_binary(const uint8_t* data, size_t size) {
    std::vector<uint8_t> frame;
    frame.push_back(0x82);
    
    if (size <= 125) {
        frame.push_back(static_cast<uint8_t>(size));
    } else if (size <= 65535) {
        frame.push_back(126);
        frame.push_back((size >> 8) & 0xFF);
        frame.push_back(size & 0xFF);
    } else {
        frame.push_back(127);
        for (int i = 7; i >= 0; i--) {
            frame.push_back((size >> (i * 8)) & 0xFF);
        }
    }
    
    frame.insert(frame.end(), data, data + size);
    
    for (auto& client : clients_) {
        if (client.active && client.socket != INVALID_SOCKET) {
            send(client.socket, reinterpret_cast<const char*>(frame.data()), 
                 frame.size(), 0);
        }
    }
}

size_t WebSocketServer::client_count() const {
    size_t count = 0;
    for (const auto& client : clients_) {
        if (client.active) count++;
    }
    return count;
}
