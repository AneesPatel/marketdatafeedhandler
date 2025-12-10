#pragma once
#include <cstdint>

#ifdef _WIN32
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

namespace marketdata {

inline uint64_t rdtsc() {
#ifdef _WIN32
    return __rdtsc();
#else
    uint32_t lo, hi;
    __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return (static_cast<uint64_t>(hi) << 32) | lo;
#endif
}

inline uint64_t rdtscp() {
#ifdef _WIN32
    unsigned int aux;
    return __rdtscp(&aux);
#else
    uint32_t lo, hi;
    __asm__ volatile("rdtscp" : "=a"(lo), "=d"(hi) :: "rcx");
    return (static_cast<uint64_t>(hi) << 32) | lo;
#endif
}

class Timer {
public:
    Timer() : freq_mhz_(0.0) {}
    
    void calibrate() {
#ifdef _WIN32
        LARGE_INTEGER freq, start, end;
        QueryPerformanceFrequency(&freq);
        QueryPerformanceCounter(&start);
        uint64_t tsc_start = rdtsc();
        Sleep(100);
        uint64_t tsc_end = rdtsc();
        QueryPerformanceCounter(&end);
        double elapsed_sec = static_cast<double>(end.QuadPart - start.QuadPart) / freq.QuadPart;
        freq_mhz_ = static_cast<double>(tsc_end - tsc_start) / (elapsed_sec * 1e6);
#else
        uint64_t tsc_start = rdtsc();
        usleep(100000);
        uint64_t tsc_end = rdtsc();
        freq_mhz_ = static_cast<double>(tsc_end - tsc_start) / 100000.0;
#endif
    }
    
    double cycles_to_ns(uint64_t cycles) const {
        return static_cast<double>(cycles) / freq_mhz_ * 1000.0;
    }
    
    double get_freq_mhz() const { return freq_mhz_; }
    
private:
    double freq_mhz_;
};

}
