#include "enhanced_order_book.hpp"
#include <algorithm>
#include <cmath>

EnhancedOrderBook::EnhancedOrderBook(const std::string& symbol)
    : symbol_(symbol)
    , last_update_time_(0)
    , message_count_(0)
    , total_bid_quantity_(0)
    , total_ask_quantity_(0) {}

bool EnhancedOrderBook::add_order(uint64_t order_id, char side, int64_t price,
                                  uint64_t quantity, uint64_t timestamp) {
    if (orders_.find(order_id) != orders_.end()) {
        return false;
    }
    
    Order order(order_id, symbol_, side, price, quantity, timestamp);
    orders_[order_id] = order;
    
    add_to_price_level(order);
    
    last_update_time_ = timestamp;
    message_count_++;
    
    return true;
}

bool EnhancedOrderBook::modify_order(uint64_t order_id, uint64_t new_quantity, 
                                     uint64_t timestamp) {
    auto it = orders_.find(order_id);
    if (it == orders_.end()) {
        return false;
    }
    
    Order& order = it->second;
    remove_from_price_level(order);
    
    order.quantity = new_quantity;
    order.timestamp = timestamp;
    
    if (new_quantity > 0) {
        add_to_price_level(order);
    } else {
        orders_.erase(it);
    }
    
    last_update_time_ = timestamp;
    message_count_++;
    
    return true;
}

bool EnhancedOrderBook::cancel_order(uint64_t order_id, uint64_t cancelled_quantity,
                                     uint64_t timestamp) {
    auto it = orders_.find(order_id);
    if (it == orders_.end()) {
        return false;
    }
    
    Order& order = it->second;
    remove_from_price_level(order);
    
    if (order.quantity > cancelled_quantity) {
        order.quantity -= cancelled_quantity;
        order.timestamp = timestamp;
        add_to_price_level(order);
    } else {
        orders_.erase(it);
    }
    
    last_update_time_ = timestamp;
    message_count_++;
    
    return true;
}

bool EnhancedOrderBook::delete_order(uint64_t order_id, uint64_t timestamp) {
    auto it = orders_.find(order_id);
    if (it == orders_.end()) {
        return false;
    }
    
    remove_from_price_level(it->second);
    orders_.erase(it);
    
    last_update_time_ = timestamp;
    message_count_++;
    
    return true;
}

bool EnhancedOrderBook::execute_order(uint64_t order_id, uint64_t executed_quantity,
                                      uint64_t timestamp) {
    auto it = orders_.find(order_id);
    if (it == orders_.end()) {
        return false;
    }
    
    Order& order = it->second;
    remove_from_price_level(order);
    
    if (order.quantity > executed_quantity) {
        order.quantity -= executed_quantity;
        order.timestamp = timestamp;
        add_to_price_level(order);
    } else {
        orders_.erase(it);
    }
    
    last_update_time_ = timestamp;
    message_count_++;
    
    return true;
}

bool EnhancedOrderBook::replace_order(uint64_t old_order_id, uint64_t new_order_id,
                                      uint64_t new_quantity, int64_t new_price,
                                      uint64_t timestamp) {
    auto it = orders_.find(old_order_id);
    if (it == orders_.end()) {
        return false;
    }
    
    char side = it->second.side;
    delete_order(old_order_id, timestamp);
    return add_order(new_order_id, side, new_price, new_quantity, timestamp);
}

std::optional<int64_t> EnhancedOrderBook::best_bid() const {
    if (bids_.empty()) return std::nullopt;
    return bids_.begin()->first;
}

std::optional<int64_t> EnhancedOrderBook::best_ask() const {
    if (asks_.empty()) return std::nullopt;
    return asks_.begin()->first;
}

std::optional<uint64_t> EnhancedOrderBook::best_bid_size() const {
    if (bids_.empty()) return std::nullopt;
    return bids_.begin()->second.size;
}

std::optional<uint64_t> EnhancedOrderBook::best_ask_size() const {
    if (asks_.empty()) return std::nullopt;
    return asks_.begin()->second.size;
}

double EnhancedOrderBook::spread() const {
    auto bid = best_bid();
    auto ask = best_ask();
    
    if (!bid || !ask) return 0.0;
    
    return (*ask - *bid) / 10000.0;
}

double EnhancedOrderBook::imbalance() const {
    double total = static_cast<double>(total_bid_quantity_ + total_ask_quantity_);
    if (total == 0.0) return 0.0;
    
    return (static_cast<double>(total_bid_quantity_) - 
            static_cast<double>(total_ask_quantity_)) / total;
}

double EnhancedOrderBook::mid_price() const {
    auto bid = best_bid();
    auto ask = best_ask();
    
    if (!bid || !ask) return 0.0;
    
    return (*bid + *ask) / 20000.0;
}

OrderBookSnapshot EnhancedOrderBook::snapshot() const {
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

std::vector<PriceLevel> EnhancedOrderBook::get_bid_depth(size_t levels) const {
    std::vector<PriceLevel> result;
    result.reserve(std::min(levels, bids_.size()));
    
    size_t count = 0;
    for (const auto& [price, level] : bids_) {
        if (count >= levels) break;
        result.push_back(level);
        count++;
    }
    
    return result;
}

std::vector<PriceLevel> EnhancedOrderBook::get_ask_depth(size_t levels) const {
    std::vector<PriceLevel> result;
    result.reserve(std::min(levels, asks_.size()));
    
    size_t count = 0;
    for (const auto& [price, level] : asks_) {
        if (count >= levels) break;
        result.push_back(level);
        count++;
    }
    
    return result;
}

bool EnhancedOrderBook::has_crossing() const {
    auto bid = best_bid();
    auto ask = best_ask();
    
    if (!bid || !ask) return false;
    
    return *bid >= *ask;
}

void EnhancedOrderBook::clear() {
    bids_.clear();
    asks_.clear();
    orders_.clear();
    last_update_time_ = 0;
    message_count_ = 0;
    total_bid_quantity_ = 0;
    total_ask_quantity_ = 0;
}

void EnhancedOrderBook::remove_from_price_level(const Order& order) {
    if (order.side == 'B' || order.side == 'b') {
        auto it = bids_.find(order.price);
        if (it != bids_.end()) {
            auto& level = it->second;
            if (level.size >= order.quantity) {
                level.size -= order.quantity;
                total_bid_quantity_ -= order.quantity;
            }
            if (level.order_count > 0) {
                level.order_count--;
            }
            if (level.size == 0 || level.order_count == 0) {
                bids_.erase(it);
            }
        }
    } else {
        auto it = asks_.find(order.price);
        if (it != asks_.end()) {
            auto& level = it->second;
            if (level.size >= order.quantity) {
                level.size -= order.quantity;
                total_ask_quantity_ -= order.quantity;
            }
            if (level.order_count > 0) {
                level.order_count--;
            }
            if (level.size == 0 || level.order_count == 0) {
                asks_.erase(it);
            }
        }
    }
}

void EnhancedOrderBook::add_to_price_level(const Order& order) {
    if (order.side == 'B' || order.side == 'b') {
        auto& level = bids_[order.price];
        level.price = order.price;
        level.size += order.quantity;
        level.order_count++;
        total_bid_quantity_ += order.quantity;
    } else {
        auto& level = asks_[order.price];
        level.price = order.price;
        level.size += order.quantity;
        level.order_count++;
        total_ask_quantity_ += order.quantity;
    }
}
