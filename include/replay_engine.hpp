#pragma once

#include "tick_recorder.hpp"
#include <thread>
#include <atomic>
#include <functional>
#include <chrono>

class ReplayEngine {
public:
    enum class Speed {
        Real1x,
        Fast10x,
        Fast100x,
        MaxSpeed
    };
    
    using MessageCallback = std::function<void(const TickRecord&)>;
    
private:
    std::string filename_;
    Speed speed_;
    MessageCallback callback_;
    
    std::thread replay_thread_;
    std::atomic<bool> running_;
    std::atomic<bool> paused_;
    std::atomic<uint64_t> messages_replayed_;
    
    void replay_loop();
    
public:
    explicit ReplayEngine(const std::string& filename);
    ~ReplayEngine();
    
    void set_callback(MessageCallback callback);
    void set_speed(Speed speed);
    
    void start();
    void stop();
    void pause();
    void resume();
    
    bool is_running() const { return running_; }
    bool is_paused() const { return paused_; }
    uint64_t messages_replayed() const { return messages_replayed_; }
};
