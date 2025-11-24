#include "tick_recorder.hpp"
#include <cstring>
#include <stdexcept>

TickRecorder::TickRecorder(const std::string& filename, bool use_mmap)
    : filename_(filename)
    , record_count_(0)
    , mapped_ptr_(nullptr)
    , mapped_size_(0)
    , use_mmap_(use_mmap) {
    
    if (use_mmap_) {
#ifdef _WIN32
        file_handle_ = CreateFileA(filename.c_str(), 
            GENERIC_READ | GENERIC_WRITE,
            0, nullptr, CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL, nullptr);
        
        if (file_handle_ == INVALID_HANDLE_VALUE) {
            use_mmap_ = false;
        }
#else
        fd_ = open(filename.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644);
        if (fd_ == -1) {
            use_mmap_ = false;
        }
#endif
    }
    
    if (!use_mmap_) {
        file_.open(filename, std::ios::binary | std::ios::trunc);
    }
}

TickRecorder::~TickRecorder() {
    flush();
    
    if (use_mmap_) {
#ifdef _WIN32
        if (mapped_ptr_) {
            UnmapViewOfFile(mapped_ptr_);
        }
        if (mapping_handle_) {
            CloseHandle(mapping_handle_);
        }
        if (file_handle_ != INVALID_HANDLE_VALUE) {
            CloseHandle(file_handle_);
        }
#else
        if (mapped_ptr_ && mapped_ptr_ != MAP_FAILED) {
            munmap(mapped_ptr_, mapped_size_);
        }
        if (fd_ != -1) {
            close(fd_);
        }
#endif
    } else {
        file_.close();
    }
}

void TickRecorder::record_trade(uint64_t timestamp, const std::string& symbol,
                                int64_t price, uint64_t size, uint8_t side) {
    TickRecord record{};
    record.timestamp = timestamp;
    std::memset(record.symbol, 0, sizeof(record.symbol));
    std::memcpy(record.symbol, symbol.c_str(), 
                std::min(symbol.size(), sizeof(record.symbol)));
    record.price = price;
    record.size = size;
    record.side = side;
    record.flags = 0x01;
    
    write_record(record);
}

void TickRecorder::record_quote(uint64_t timestamp, const std::string& symbol,
                                int64_t bid_price, uint64_t bid_size,
                                int64_t ask_price, uint64_t ask_size) {
    TickRecord bid_record{};
    bid_record.timestamp = timestamp;
    std::memset(bid_record.symbol, 0, sizeof(bid_record.symbol));
    std::memcpy(bid_record.symbol, symbol.c_str(),
                std::min(symbol.size(), sizeof(bid_record.symbol)));
    bid_record.price = bid_price;
    bid_record.size = bid_size;
    bid_record.side = 0;
    bid_record.flags = 0x02;
    
    TickRecord ask_record = bid_record;
    ask_record.price = ask_price;
    ask_record.size = ask_size;
    ask_record.side = 1;
    
    write_record(bid_record);
    write_record(ask_record);
}

void TickRecorder::write_record(const TickRecord& record) {
    if (use_mmap_) {
        remap_if_needed();
        
        if (mapped_ptr_) {
            TickRecord* dest = static_cast<TickRecord*>(mapped_ptr_) + record_count_;
            *dest = record;
        }
    } else {
        file_.write(reinterpret_cast<const char*>(&record), sizeof(record));
    }
    
    record_count_++;
}

void TickRecorder::remap_if_needed() {
    size_t needed_size = (record_count_ + 1) * sizeof(TickRecord);
    
    if (needed_size <= mapped_size_) {
        return;
    }
    
    size_t new_size = std::max(needed_size * 2, size_t(1024 * 1024));
    
#ifdef _WIN32
    if (mapped_ptr_) {
        UnmapViewOfFile(mapped_ptr_);
        mapped_ptr_ = nullptr;
    }
    if (mapping_handle_) {
        CloseHandle(mapping_handle_);
        mapping_handle_ = nullptr;
    }
    
    LARGE_INTEGER li;
    li.QuadPart = new_size;
    
    mapping_handle_ = CreateFileMappingA(file_handle_, nullptr,
        PAGE_READWRITE, li.HighPart, li.LowPart, nullptr);
    
    if (mapping_handle_) {
        mapped_ptr_ = MapViewOfFile(mapping_handle_, FILE_MAP_WRITE, 0, 0, 0);
        if (mapped_ptr_) {
            mapped_size_ = new_size;
        }
    }
#else
    if (mapped_ptr_ && mapped_ptr_ != MAP_FAILED) {
        munmap(mapped_ptr_, mapped_size_);
        mapped_ptr_ = nullptr;
    }
    
    if (ftruncate(fd_, new_size) == 0) {
        mapped_ptr_ = mmap(nullptr, new_size, PROT_READ | PROT_WRITE,
                          MAP_SHARED, fd_, 0);
        if (mapped_ptr_ != MAP_FAILED) {
            mapped_size_ = new_size;
        }
    }
#endif
}

void TickRecorder::flush() {
    if (use_mmap_) {
#ifdef _WIN32
        if (mapped_ptr_) {
            FlushViewOfFile(mapped_ptr_, 0);
        }
        if (file_handle_ != INVALID_HANDLE_VALUE) {
            SetFilePointer(file_handle_, record_count_ * sizeof(TickRecord), 
                          nullptr, FILE_BEGIN);
            SetEndOfFile(file_handle_);
        }
#else
        if (mapped_ptr_ && mapped_ptr_ != MAP_FAILED) {
            msync(mapped_ptr_, mapped_size_, MS_SYNC);
        }
        if (fd_ != -1) {
            ftruncate(fd_, record_count_ * sizeof(TickRecord));
        }
#endif
    } else {
        file_.flush();
    }
}

TickReader::TickReader(const std::string& filename)
    : filename_(filename)
    , mapped_ptr_(nullptr)
    , file_size_(0)
    , current_offset_(0) {
    
#ifdef _WIN32
    file_handle_ = CreateFileA(filename.c_str(), GENERIC_READ,
        FILE_SHARE_READ, nullptr, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, nullptr);
    
    if (file_handle_ == INVALID_HANDLE_VALUE) {
        throw std::runtime_error("Cannot open file");
    }
    
    LARGE_INTEGER li;
    GetFileSizeEx(file_handle_, &li);
    file_size_ = li.QuadPart;
    
    mapping_handle_ = CreateFileMappingA(file_handle_, nullptr,
        PAGE_READONLY, 0, 0, nullptr);
    
    if (mapping_handle_) {
        mapped_ptr_ = MapViewOfFile(mapping_handle_, FILE_MAP_READ, 0, 0, 0);
    }
#else
    fd_ = open(filename.c_str(), O_RDONLY);
    if (fd_ == -1) {
        throw std::runtime_error("Cannot open file");
    }
    
    file_size_ = lseek(fd_, 0, SEEK_END);
    lseek(fd_, 0, SEEK_SET);
    
    mapped_ptr_ = mmap(nullptr, file_size_, PROT_READ, MAP_PRIVATE, fd_, 0);
    if (mapped_ptr_ == MAP_FAILED) {
        mapped_ptr_ = nullptr;
    }
#endif
}

TickReader::~TickReader() {
#ifdef _WIN32
    if (mapped_ptr_) {
        UnmapViewOfFile(mapped_ptr_);
    }
    if (mapping_handle_) {
        CloseHandle(mapping_handle_);
    }
    if (file_handle_ != INVALID_HANDLE_VALUE) {
        CloseHandle(file_handle_);
    }
#else
    if (mapped_ptr_ && mapped_ptr_ != MAP_FAILED) {
        munmap(const_cast<void*>(mapped_ptr_), file_size_);
    }
    if (fd_ != -1) {
        close(fd_);
    }
#endif
}

bool TickReader::read_next(TickRecord& record) {
    if (current_offset_ + sizeof(TickRecord) > file_size_) {
        return false;
    }
    
    const TickRecord* src = static_cast<const TickRecord*>(mapped_ptr_);
    record = src[current_offset_ / sizeof(TickRecord)];
    current_offset_ += sizeof(TickRecord);
    
    return true;
}

void TickReader::reset() {
    current_offset_ = 0;
}

size_t TickReader::total_records() const {
    return file_size_ / sizeof(TickRecord);
}

bool TickReader::has_more() const {
    return current_offset_ < file_size_;
}
