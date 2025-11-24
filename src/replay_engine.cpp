#include "replay_engine.hpp"
#include <thread>

ReplayEngine::ReplayEngine(const std::string& filename)
    : filename_(filename)
    , speed_(Speed::Real1x)
    , running_(false)
    , paused_(false)
    , messages_replayed_(0) {}

ReplayEngine::~ReplayEngine() {
    stop();
}

void ReplayEngine::set_callback(MessageCallback callback) {
    callback_ = callback;
}

void ReplayEngine::set_speed(Speed speed) {
    speed_ = speed;
}

void ReplayEngine::start() {
    if (running_) return;
    
    running_ = true;
    paused_ = false;
    messages_replayed_ = 0;
    
    replay_thread_ = std::thread(&ReplayEngine::replay_loop, this);
}

void ReplayEngine::stop() {
    running_ = false;
    
    if (replay_thread_.joinable()) {
        replay_thread_.join();
    }
}

void ReplayEngine::pause() {
    paused_ = true;
}

void ReplayEngine::resume() {
    paused_ = false;
}

void ReplayEngine::replay_loop() {
    TickReader reader(filename_);
    
    TickRecord prev_record{};
    TickRecord current_record{};
    
    bool first = true;
    
    while (running_ && reader.has_more()) {
        while (paused_ && running_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        if (!running_) break;
        
        if (!reader.read_next(current_record)) {
            break;
        }
        
        if (callback_) {
            callback_(current_record);
        }
        
        if (!first && speed_ != Speed::MaxSpeed) {
            uint64_t time_delta = current_record.timestamp - prev_record.timestamp;
            
            if (time_delta > 0) {
                uint64_t sleep_time = time_delta;
                
                switch (speed_) {
                    case Speed::Real1x:
                        break;
                    case Speed::Fast10x:
                        sleep_time /= 10;
                        break;
                    case Speed::Fast100x:
                        sleep_time /= 100;
                        break;
                    default:
                        sleep_time = 0;
                        break;
                }
                
                if (sleep_time > 0 && sleep_time < 1000000000) {
                    std::this_thread::sleep_for(
                        std::chrono::nanoseconds(sleep_time));
                }
            }
        }
        
        prev_record = current_record;
        first = false;
        messages_replayed_++;
    }
    
    running_ = false;
}
