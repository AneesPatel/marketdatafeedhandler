#pragma once

#include <map>
#include <string>
#include <cstdint>
#include <optional>
#include <vector>

struct PriceLevel {
    int64_t price;
    uint64_t size;
    uint64_t order_count;
    
    PriceLevel() : price(0), size(0), order_count(0) {}
    PriceLevel(int64_t p, uint64_t s) : price(p), size(s), order_count(1) {}
};

struct OrderBookSnapshot {
    std::string symbol;
    uint64_t timestamp;
    int64_t best_bid;
    uint64_t best_bid_size;
    int64_t best_ask;
    uint64_t best_ask_size;
    double spread;
    double imbalance;
    size_t bid_levels;
    size_t ask_levels;
};

class OrderBook {
    std::string symbol_;
    std::map<int64_t, PriceLevel, std::greater<int64_t>> bids_;
    std::map<int64_t, PriceLevel, std::less<int64_t>> asks_;
    
    uint64_t last_update_time_;
    uint64_t message_count_;
    
public:
    explicit OrderBook(const std::string& symbol);
    
    void add_bid(int64_t price, uint64_t size, uint64_t timestamp);
    void add_ask(int64_t price, uint64_t size, uint64_t timestamp);
    void modify_bid(int64_t price, uint64_t size, uint64_t timestamp);
    void modify_ask(int64_t price, uint64_t size, uint64_t timestamp);
    void remove_bid(int64_t price, uint64_t timestamp);
    void remove_ask(int64_t price, uint64_t timestamp);
    
    void execute_bid(int64_t price, uint64_t size, uint64_t timestamp);
    void execute_ask(int64_t price, uint64_t size, uint64_t timestamp);
    
    std::optional<int64_t> best_bid() const;
    std::optional<int64_t> best_ask() const;
    std::optional<uint64_t> best_bid_size() const;
    std::optional<uint64_t> best_ask_size() const;
    
    double spread() const;
    double imbalance() const;
    
    OrderBookSnapshot snapshot() const;
    
    const std::string& symbol() const { return symbol_; }
    uint64_t message_count() const { return message_count_; }
    
    size_t bid_levels() const { return bids_.size(); }
    size_t ask_levels() const { return asks_.size(); }
    
    void clear();
};

class OrderBookManager {
    std::map<std::string, OrderBook> books_;
    
public:
    OrderBook& get_or_create(const std::string& symbol);
    OrderBook* get(const std::string& symbol);
    const OrderBook* get(const std::string& symbol) const;
    
    std::vector<std::string> symbols() const;
    size_t size() const { return books_.size(); }
    
    void clear();
};
