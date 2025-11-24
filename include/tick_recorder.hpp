#pragma once

#include <fstream>
#include <string>
#include <cstdint>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#endif

struct TickRecord {
    uint64_t timestamp;
    char symbol[8];
    int64_t price;
    uint64_t size;
    uint8_t side;
    uint8_t flags;
    uint16_t padding;
};

class TickRecorder {
    std::string filename_;
    std::ofstream file_;
    uint64_t record_count_;
    
#ifdef _WIN32
    HANDLE file_handle_;
    HANDLE mapping_handle_;
    void* mapped_ptr_;
#else
    int fd_;
    void* mapped_ptr_;
#endif
    
    size_t mapped_size_;
    bool use_mmap_;
    
public:
    explicit TickRecorder(const std::string& filename, bool use_mmap = true);
    ~TickRecorder();
    
    void record_trade(uint64_t timestamp, const std::string& symbol, 
                     int64_t price, uint64_t size, uint8_t side);
    
    void record_quote(uint64_t timestamp, const std::string& symbol,
                     int64_t bid_price, uint64_t bid_size,
                     int64_t ask_price, uint64_t ask_size);
    
    void flush();
    uint64_t count() const { return record_count_; }
    
private:
    void write_record(const TickRecord& record);
    void remap_if_needed();
};

class TickReader {
    std::string filename_;
    
#ifdef _WIN32
    HANDLE file_handle_;
    HANDLE mapping_handle_;
    const void* mapped_ptr_;
#else
    int fd_;
    const void* mapped_ptr_;
#endif
    
    size_t file_size_;
    size_t current_offset_;
    
public:
    explicit TickReader(const std::string& filename);
    ~TickReader();
    
    bool read_next(TickRecord& record);
    void reset();
    size_t total_records() const;
    bool has_more() const;
};
