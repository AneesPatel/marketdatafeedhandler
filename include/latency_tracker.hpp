#pragma once

#include <cstdint>

#ifdef _WIN32
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

namespace perf {

inline uint64_t rdtsc() {
#ifdef _WIN32
    return __rdtsc();
#else
    uint32_t lo, hi;
    __asm__ volatile ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
#endif
}

inline uint64_t rdtsc_start() {
    uint32_t aux;
#ifdef _WIN32
    return __rdtscp(&aux);
#else
    uint32_t lo, hi;
    __asm__ volatile ("rdtscp" : "=a"(lo), "=d"(hi), "=c"(aux));
    return ((uint64_t)hi << 32) | lo;
#endif
}

inline uint64_t rdtsc_end() {
    uint32_t lo, hi;
#ifdef _WIN32
    _mm_lfence();
    return __rdtsc();
#else
    __asm__ volatile ("rdtscp" : "=a"(lo), "=d"(hi) :: "rcx");
    __asm__ volatile ("lfence");
    return ((uint64_t)hi << 32) | lo;
#endif
}

class LatencyHistogram {
    static constexpr size_t NUM_BUCKETS = 1024;
    uint64_t buckets_[NUM_BUCKETS];
    uint64_t count_;
    uint64_t min_;
    uint64_t max_;
    uint64_t sum_;
    
public:
    LatencyHistogram() : count_(0), min_(UINT64_MAX), max_(0), sum_(0) {
        for (size_t i = 0; i < NUM_BUCKETS; ++i) {
            buckets_[i] = 0;
        }
    }
    
    void record(uint64_t cycles) {
        count_++;
        sum_ += cycles;
        
        if (cycles < min_) min_ = cycles;
        if (cycles > max_) max_ = cycles;
        
        size_t bucket = cycles < NUM_BUCKETS ? cycles : NUM_BUCKETS - 1;
        buckets_[bucket]++;
    }
    
    double average() const {
        return count_ > 0 ? static_cast<double>(sum_) / count_ : 0.0;
    }
    
    uint64_t percentile(double p) const {
        if (count_ == 0) return 0;
        
        uint64_t target = static_cast<uint64_t>(count_ * p);
        uint64_t accumulated = 0;
        
        for (size_t i = 0; i < NUM_BUCKETS; ++i) {
            accumulated += buckets_[i];
            if (accumulated >= target) {
                return i;
            }
        }
        
        return NUM_BUCKETS - 1;
    }
    
    uint64_t min() const { return min_; }
    uint64_t max() const { return max_; }
    uint64_t count() const { return count_; }
    
    void reset() {
        count_ = 0;
        min_ = UINT64_MAX;
        max_ = 0;
        sum_ = 0;
        for (size_t i = 0; i < NUM_BUCKETS; ++i) {
            buckets_[i] = 0;
        }
    }
};

inline double cycles_to_ns(uint64_t cycles, double cpu_freq_ghz = 3.0) {
    return cycles / cpu_freq_ghz;
}

inline double cycles_to_us(uint64_t cycles, double cpu_freq_ghz = 3.0) {
    return cycles / (cpu_freq_ghz * 1000.0);
}

}
