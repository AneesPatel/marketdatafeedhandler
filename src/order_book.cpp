#include "order_book.hpp"
#include <algorithm>
#include <cmath>

OrderBook::OrderBook(const std::string& symbol)
    : symbol_(symbol)
    , last_update_time_(0)
    , message_count_(0) {}

void OrderBook::add_bid(int64_t price, uint64_t size, uint64_t timestamp) {
    auto& level = bids_[price];
    level.price = price;
    level.size += size;
    level.order_count++;
    last_update_time_ = timestamp;
    message_count_++;
}

void OrderBook::add_ask(int64_t price, uint64_t size, uint64_t timestamp) {
    auto& level = asks_[price];
    level.price = price;
    level.size += size;
    level.order_count++;
    last_update_time_ = timestamp;
    message_count_++;
}

void OrderBook::modify_bid(int64_t price, uint64_t size, uint64_t timestamp) {
    auto it = bids_.find(price);
    if (it != bids_.end()) {
        it->second.size = size;
        if (size == 0) {
            bids_.erase(it);
        }
    } else if (size > 0) {
        bids_[price] = PriceLevel(price, size);
    }
    last_update_time_ = timestamp;
    message_count_++;
}

void OrderBook::modify_ask(int64_t price, uint64_t size, uint64_t timestamp) {
    auto it = asks_.find(price);
    if (it != asks_.end()) {
        it->second.size = size;
        if (size == 0) {
            asks_.erase(it);
        }
    } else if (size > 0) {
        asks_[price] = PriceLevel(price, size);
    }
    last_update_time_ = timestamp;
    message_count_++;
}

void OrderBook::remove_bid(int64_t price, uint64_t timestamp) {
    bids_.erase(price);
    last_update_time_ = timestamp;
    message_count_++;
}

void OrderBook::remove_ask(int64_t price, uint64_t timestamp) {
    asks_.erase(price);
    last_update_time_ = timestamp;
    message_count_++;
}

void OrderBook::execute_bid(int64_t price, uint64_t size, uint64_t timestamp) {
    auto it = bids_.find(price);
    if (it != bids_.end()) {
        if (it->second.size > size) {
            it->second.size -= size;
        } else {
            bids_.erase(it);
        }
    }
    last_update_time_ = timestamp;
    message_count_++;
}

void OrderBook::execute_ask(int64_t price, uint64_t size, uint64_t timestamp) {
    auto it = asks_.find(price);
    if (it != asks_.end()) {
        if (it->second.size > size) {
            it->second.size -= size;
        } else {
            asks_.erase(it);
        }
    }
    last_update_time_ = timestamp;
    message_count_++;
}

std::optional<int64_t> OrderBook::best_bid() const {
    if (bids_.empty()) return std::nullopt;
    return bids_.begin()->first;
}

std::optional<int64_t> OrderBook::best_ask() const {
    if (asks_.empty()) return std::nullopt;
    return asks_.begin()->first;
}

std::optional<uint64_t> OrderBook::best_bid_size() const {
    if (bids_.empty()) return std::nullopt;
    return bids_.begin()->second.size;
}

std::optional<uint64_t> OrderBook::best_ask_size() const {
    if (asks_.empty()) return std::nullopt;
    return asks_.begin()->second.size;
}

double OrderBook::spread() const {
    auto bid = best_bid();
    auto ask = best_ask();
    
    if (!bid || !ask) return 0.0;
    
    return (*ask - *bid) / 10000.0;
}

double OrderBook::imbalance() const {
    auto bid_size = best_bid_size();
    auto ask_size = best_ask_size();
    
    if (!bid_size || !ask_size) return 0.0;
    
    double total = static_cast<double>(*bid_size + *ask_size);
    if (total == 0.0) return 0.0;
    
    return (static_cast<double>(*bid_size) - static_cast<double>(*ask_size)) / total;
}

OrderBookSnapshot OrderBook::snapshot() const {
    OrderBookSnapshot snap;
    snap.symbol = symbol_;
    snap.timestamp = last_update_time_;
    
    auto bid = best_bid();
    auto ask = best_ask();
    auto bid_sz = best_bid_size();
    auto ask_sz = best_ask_size();
    
    snap.best_bid = bid.value_or(0);
    snap.best_ask = ask.value_or(0);
    snap.best_bid_size = bid_sz.value_or(0);
    snap.best_ask_size = ask_sz.value_or(0);
    snap.spread = spread();
    snap.imbalance = imbalance();
    snap.bid_levels = bids_.size();
    snap.ask_levels = asks_.size();
    
    return snap;
}

void OrderBook::clear() {
    bids_.clear();
    asks_.clear();
    last_update_time_ = 0;
    message_count_ = 0;
}

OrderBook& OrderBookManager::get_or_create(const std::string& symbol) {
    auto it = books_.find(symbol);
    if (it == books_.end()) {
        it = books_.emplace(symbol, OrderBook(symbol)).first;
    }
    return it->second;
}

OrderBook* OrderBookManager::get(const std::string& symbol) {
    auto it = books_.find(symbol);
    return it != books_.end() ? &it->second : nullptr;
}

const OrderBook* OrderBookManager::get(const std::string& symbol) const {
    auto it = books_.find(symbol);
    return it != books_.end() ? &it->second : nullptr;
}

std::vector<std::string> OrderBookManager::symbols() const {
    std::vector<std::string> result;
    result.reserve(books_.size());
    for (const auto& [symbol, _] : books_) {
        result.push_back(symbol);
    }
    return result;
}

void OrderBookManager::clear() {
    books_.clear();
}
