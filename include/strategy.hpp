#pragma once

#include "order_book.hpp"
#include <functional>

class TradingStrategy {
public:
    using OrderSignal = std::function<void(const std::string&, double, uint64_t, bool)>;
    
private:
    OrderSignal signal_callback_;
    
    double spread_threshold_;
    double imbalance_threshold_;
    
    struct PositionInfo {
        int64_t quantity;
        double avg_price;
    };
    
    std::map<std::string, PositionInfo> positions_;
    
public:
    TradingStrategy();
    
    void set_signal_callback(OrderSignal callback);
    void set_spread_threshold(double threshold);
    void set_imbalance_threshold(double threshold);
    
    void on_quote_update(const OrderBookSnapshot& snapshot);
    void on_trade(const std::string& symbol, int64_t price, uint64_t size);
    
    const std::map<std::string, PositionInfo>& positions() const { 
        return positions_; 
    }
};
