#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "marketdata_parser.hpp"
#include "timer.hpp"
#include "latency_histogram.hpp"
#include <fstream>
#include <vector>
#include <iostream>
#include <iomanip>
#include <cstdint>

static const size_t MESSAGE_SIZE = 36;
static const size_t WARMUP_ITERATIONS = 10000;

int main() {
    std::ifstream file("data/sample_itch.bin", std::ios::binary | std::ios::ate);
    if (!file) {
        std::cerr << "Failed to open data/sample_itch.bin" << std::endl;
        return 1;
    }
    
    size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<uint8_t> buffer(file_size);
    file.read(reinterpret_cast<char*>(buffer.data()), file_size);
    file.close();
    
    size_t message_count = file_size / MESSAGE_SIZE;
    
    std::cout << "Loaded " << message_count << " messages (" << file_size << " bytes)" << std::endl;
    
    marketdata::Timer timer;
    std::cout << "Calibrating CPU frequency..." << std::endl;
    timer.calibrate();
    std::cout << "CPU frequency: " << std::fixed << std::setprecision(2) << timer.get_freq_mhz() << " MHz" << std::endl;
    
    marketdata::Order order;
    marketdata::ITCHParser parser;
    
    std::cout << "Warmup..." << std::endl;
    for (size_t i = 0; i < WARMUP_ITERATIONS && i < message_count; ++i) {
        parser.parse_add_order(buffer.data() + i * MESSAGE_SIZE, MESSAGE_SIZE, order);
    }
    
    marketdata::LatencyHistogram hist;
    
    std::cout << "Running benchmark..." << std::endl;
    
    uint64_t total_start = marketdata::rdtsc();
    
    for (size_t i = 0; i < message_count; ++i) {
        const uint8_t* msg = buffer.data() + i * MESSAGE_SIZE;
        
        uint64_t start = marketdata::rdtsc();
        parser.parse_add_order(msg, MESSAGE_SIZE, order);
        uint64_t end = marketdata::rdtsc();
        
        double latency_ns = timer.cycles_to_ns(end - start);
        hist.record(latency_ns);
    }
    
    uint64_t total_end = marketdata::rdtsc();
    double total_time_ns = timer.cycles_to_ns(total_end - total_start);
    double total_time_sec = total_time_ns / 1e9;
    double throughput = message_count / total_time_sec;
    
    std::cout << std::endl;
    std::cout << "=== BENCHMARK RESULTS ===" << std::endl;
    std::cout << std::endl;
    std::cout << "Messages parsed: " << message_count << std::endl;
    std::cout << "Total time: " << std::fixed << std::setprecision(3) << total_time_sec * 1000 << " ms" << std::endl;
    std::cout << "Throughput: " << std::fixed << std::setprecision(0) << throughput << " msgs/sec" << std::endl;
    std::cout << std::endl;
    std::cout << "Latency (nanoseconds):" << std::endl;
    std::cout << "  Min:  " << std::fixed << std::setprecision(1) << hist.min() << " ns" << std::endl;
    std::cout << "  Mean: " << std::fixed << std::setprecision(1) << hist.mean() << " ns" << std::endl;
    std::cout << "  P50:  " << std::fixed << std::setprecision(1) << hist.p50() << " ns" << std::endl;
    std::cout << "  P90:  " << std::fixed << std::setprecision(1) << hist.p90() << " ns" << std::endl;
    std::cout << "  P99:  " << std::fixed << std::setprecision(1) << hist.p99() << " ns" << std::endl;
    std::cout << "  P99.9:" << std::fixed << std::setprecision(1) << hist.p999() << " ns" << std::endl;
    std::cout << "  Max:  " << std::fixed << std::setprecision(1) << hist.max() << " ns" << std::endl;
    
    return 0;
}
