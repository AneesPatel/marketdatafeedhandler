#include <gtest/gtest.h>
#include "order_book.hpp"

class OrderBookTest : public ::testing::Test {
protected:
    OrderBook book{"AAPL"};
    
    void SetUp() override {
        book.clear();
    }
};

TEST_F(OrderBookTest, AddBidAndAsk) {
    book.add_bid(1500000, 100, 1000);
    book.add_ask(1500100, 200, 1001);
    
    ASSERT_TRUE(book.best_bid().has_value());
    ASSERT_TRUE(book.best_ask().has_value());
    
    EXPECT_EQ(*book.best_bid(), 1500000);
    EXPECT_EQ(*book.best_ask(), 1500100);
    
    EXPECT_EQ(*book.best_bid_size(), 100);
    EXPECT_EQ(*book.best_ask_size(), 200);
}

TEST_F(OrderBookTest, SpreadCalculation) {
    book.add_bid(1500000, 100, 1000);
    book.add_ask(1500100, 200, 1001);
    
    EXPECT_DOUBLE_EQ(book.spread(), 0.01);
}

TEST_F(OrderBookTest, ImbalanceCalculation) {
    book.add_bid(1500000, 300, 1000);
    book.add_ask(1500100, 100, 1001);
    
    double expected_imbalance = (300.0 - 100.0) / (300.0 + 100.0);
    EXPECT_DOUBLE_EQ(book.imbalance(), expected_imbalance);
}

TEST_F(OrderBookTest, ModifyBid) {
    book.add_bid(1500000, 100, 1000);
    EXPECT_EQ(*book.best_bid_size(), 100);
    
    book.modify_bid(1500000, 200, 1001);
    EXPECT_EQ(*book.best_bid_size(), 200);
}

TEST_F(OrderBookTest, ExecuteOrder) {
    book.add_bid(1500000, 100, 1000);
    EXPECT_EQ(*book.best_bid_size(), 100);
    
    book.execute_bid(1500000, 30, 1001);
    EXPECT_EQ(*book.best_bid_size(), 70);
    
    book.execute_bid(1500000, 70, 1002);
    EXPECT_FALSE(book.best_bid().has_value());
}

TEST_F(OrderBookTest, RemoveLevel) {
    book.add_bid(1500000, 100, 1000);
    EXPECT_TRUE(book.best_bid().has_value());
    
    book.remove_bid(1500000, 1001);
    EXPECT_FALSE(book.best_bid().has_value());
}

TEST_F(OrderBookTest, MultiplePriceLevels) {
    book.add_bid(1500000, 100, 1000);
    book.add_bid(1499900, 200, 1001);
    book.add_bid(1500100, 150, 1002);
    
    EXPECT_EQ(*book.best_bid(), 1500100);
    EXPECT_EQ(book.bid_levels(), 3);
}

TEST_F(OrderBookTest, Snapshot) {
    book.add_bid(1500000, 100, 1000);
    book.add_ask(1500100, 200, 1001);
    
    auto snap = book.snapshot();
    
    EXPECT_EQ(snap.symbol, "AAPL");
    EXPECT_EQ(snap.best_bid, 1500000);
    EXPECT_EQ(snap.best_ask, 1500100);
    EXPECT_EQ(snap.best_bid_size, 100);
    EXPECT_EQ(snap.best_ask_size, 200);
    EXPECT_GT(snap.spread, 0.0);
}

TEST_F(OrderBookTest, EmptyBook) {
    EXPECT_FALSE(book.best_bid().has_value());
    EXPECT_FALSE(book.best_ask().has_value());
    EXPECT_DOUBLE_EQ(book.spread(), 0.0);
    EXPECT_DOUBLE_EQ(book.imbalance(), 0.0);
}
