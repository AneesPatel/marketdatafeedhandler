#pragma once

#include "order_book.hpp"
#include <unordered_map>
#include <memory>

struct Order {
    uint64_t order_id;
    std::string symbol;
    char side;
    int64_t price;
    uint64_t quantity;
    uint64_t timestamp;
    
    Order() : order_id(0), side('B'), price(0), quantity(0), timestamp(0) {}
    Order(uint64_t id, const std::string& sym, char s, int64_t p, uint64_t q, uint64_t ts)
        : order_id(id), symbol(sym), side(s), price(p), quantity(q), timestamp(ts) {}
};

class EnhancedOrderBook {
    std::string symbol_;
    std::map<int64_t, PriceLevel, std::greater<int64_t>> bids_;
    std::map<int64_t, PriceLevel, std::less<int64_t>> asks_;
    std::unordered_map<uint64_t, Order> orders_;
    
    uint64_t last_update_time_;
    uint64_t message_count_;
    uint64_t total_bid_quantity_;
    uint64_t total_ask_quantity_;
    
public:
    explicit EnhancedOrderBook(const std::string& symbol);
    
    bool add_order(uint64_t order_id, char side, int64_t price, 
                   uint64_t quantity, uint64_t timestamp);
    
    bool modify_order(uint64_t order_id, uint64_t new_quantity, uint64_t timestamp);
    
    bool cancel_order(uint64_t order_id, uint64_t cancelled_quantity, uint64_t timestamp);
    
    bool delete_order(uint64_t order_id, uint64_t timestamp);
    
    bool execute_order(uint64_t order_id, uint64_t executed_quantity, uint64_t timestamp);
    
    bool replace_order(uint64_t old_order_id, uint64_t new_order_id,
                      uint64_t new_quantity, int64_t new_price, uint64_t timestamp);
    
    std::optional<int64_t> best_bid() const;
    std::optional<int64_t> best_ask() const;
    std::optional<uint64_t> best_bid_size() const;
    std::optional<uint64_t> best_ask_size() const;
    
    double spread() const;
    double imbalance() const;
    double mid_price() const;
    
    OrderBookSnapshot snapshot() const;
    
    const std::string& symbol() const { return symbol_; }
    uint64_t message_count() const { return message_count_; }
    size_t total_orders() const { return orders_.size(); }
    
    size_t bid_levels() const { return bids_.size(); }
    size_t ask_levels() const { return asks_.size(); }
    
    std::vector<PriceLevel> get_bid_depth(size_t levels) const;
    std::vector<PriceLevel> get_ask_depth(size_t levels) const;
    
    bool has_crossing() const;
    
    void clear();
    
private:
    void remove_from_price_level(const Order& order);
    void add_to_price_level(const Order& order);
};
