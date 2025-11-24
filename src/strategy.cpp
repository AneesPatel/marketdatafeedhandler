#include "strategy.hpp"
#include <cmath>

TradingStrategy::TradingStrategy()
    : spread_threshold_(0.01)
    , imbalance_threshold_(0.3) {}

void TradingStrategy::set_signal_callback(OrderSignal callback) {
    signal_callback_ = callback;
}

void TradingStrategy::set_spread_threshold(double threshold) {
    spread_threshold_ = threshold;
}

void TradingStrategy::set_imbalance_threshold(double threshold) {
    imbalance_threshold_ = threshold;
}

void TradingStrategy::on_quote_update(const OrderBookSnapshot& snapshot) {
    if (snapshot.best_bid == 0 || snapshot.best_ask == 0) {
        return;
    }
    
    if (snapshot.spread < spread_threshold_) {
        return;
    }
    
    if (std::abs(snapshot.imbalance) > imbalance_threshold_) {
        bool buy_signal = snapshot.imbalance > 0;
        
        double price = buy_signal ? 
            snapshot.best_ask / 10000.0 : 
            snapshot.best_bid / 10000.0;
        
        uint64_t size = buy_signal ? 
            snapshot.best_ask_size : 
            snapshot.best_bid_size;
        
        size = std::min(size, uint64_t(100));
        
        if (signal_callback_) {
            signal_callback_(snapshot.symbol, price, size, buy_signal);
        }
        
        auto& pos = positions_[snapshot.symbol];
        if (buy_signal) {
            double total_cost = pos.avg_price * pos.quantity + price * size;
            pos.quantity += size;
            if (pos.quantity > 0) {
                pos.avg_price = total_cost / pos.quantity;
            }
        } else {
            pos.quantity -= size;
            if (pos.quantity == 0) {
                pos.avg_price = 0;
            }
        }
    }
}

void TradingStrategy::on_trade(const std::string& symbol, int64_t price, uint64_t size) {
    (void)symbol;
    (void)price;
    (void)size;
}
