#pragma once
#include <cstdint>
#include <vector>
#include <algorithm>
#include <cmath>

namespace marketdata {

class LatencyHistogram {
public:
    LatencyHistogram() {
        samples_.reserve(100000);
    }
    
    void record(double latency_ns) {
        samples_.push_back(latency_ns);
    }
    
    void clear() {
        samples_.clear();
    }
    
    size_t count() const {
        return samples_.size();
    }
    
    double min() const {
        if (samples_.empty()) return 0.0;
        return *std::min_element(samples_.begin(), samples_.end());
    }
    
    double max() const {
        if (samples_.empty()) return 0.0;
        return *std::max_element(samples_.begin(), samples_.end());
    }
    
    double mean() const {
        if (samples_.empty()) return 0.0;
        double sum = 0.0;
        for (auto s : samples_) sum += s;
        return sum / samples_.size();
    }
    
    double percentile(double p) {
        if (samples_.empty()) return 0.0;
        std::sort(samples_.begin(), samples_.end());
        size_t idx = static_cast<size_t>(p / 100.0 * (samples_.size() - 1));
        return samples_[idx];
    }
    
    double p50() { return percentile(50.0); }
    double p90() { return percentile(90.0); }
    double p99() { return percentile(99.0); }
    double p999() { return percentile(99.9); }
    
private:
    std::vector<double> samples_;
};

}
