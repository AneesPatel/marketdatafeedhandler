#include <gtest/gtest.h>
#include "enhanced_order_book.hpp"

class EnhancedOrderBookTest : public ::testing::Test {
protected:
    EnhancedOrderBook book{"AAPL"};
    
    void SetUp() override {
        book.clear();
    }
};

TEST_F(EnhancedOrderBookTest, AddOrder) {
    EXPECT_TRUE(book.add_order(1, 'B', 1500000, 100, 1000));
    
    ASSERT_TRUE(book.best_bid().has_value());
    EXPECT_EQ(*book.best_bid(), 1500000);
    EXPECT_EQ(*book.best_bid_size(), 100);
    EXPECT_EQ(book.total_orders(), 1);
}

TEST_F(EnhancedOrderBookTest, DuplicateOrderId) {
    EXPECT_TRUE(book.add_order(1, 'B', 1500000, 100, 1000));
    EXPECT_FALSE(book.add_order(1, 'B', 1500100, 200, 1001));
}

TEST_F(EnhancedOrderBookTest, ModifyOrder) {
    book.add_order(1, 'B', 1500000, 100, 1000);
    EXPECT_EQ(*book.best_bid_size(), 100);
    
    EXPECT_TRUE(book.modify_order(1, 200, 1001));
    EXPECT_EQ(*book.best_bid_size(), 200);
}

TEST_F(EnhancedOrderBookTest, ModifyNonExistentOrder) {
    EXPECT_FALSE(book.modify_order(999, 100, 1000));
}

TEST_F(EnhancedOrderBookTest, CancelOrder) {
    book.add_order(1, 'B', 1500000, 100, 1000);
    EXPECT_EQ(*book.best_bid_size(), 100);
    
    EXPECT_TRUE(book.cancel_order(1, 30, 1001));
    EXPECT_EQ(*book.best_bid_size(), 70);
}

TEST_F(EnhancedOrderBookTest, CancelFullOrder) {
    book.add_order(1, 'B', 1500000, 100, 1000);
    
    EXPECT_TRUE(book.cancel_order(1, 100, 1001));
    EXPECT_FALSE(book.best_bid().has_value());
    EXPECT_EQ(book.total_orders(), 0);
}

TEST_F(EnhancedOrderBookTest, DeleteOrder) {
    book.add_order(1, 'B', 1500000, 100, 1000);
    EXPECT_EQ(book.total_orders(), 1);
    
    EXPECT_TRUE(book.delete_order(1, 1001));
    EXPECT_EQ(book.total_orders(), 0);
    EXPECT_FALSE(book.best_bid().has_value());
}

TEST_F(EnhancedOrderBookTest, ExecuteOrder) {
    book.add_order(1, 'B', 1500000, 100, 1000);
    
    EXPECT_TRUE(book.execute_order(1, 40, 1001));
    EXPECT_EQ(*book.best_bid_size(), 60);
    
    EXPECT_TRUE(book.execute_order(1, 60, 1002));
    EXPECT_FALSE(book.best_bid().has_value());
}

TEST_F(EnhancedOrderBookTest, ReplaceOrder) {
    book.add_order(1, 'B', 1500000, 100, 1000);
    
    EXPECT_TRUE(book.replace_order(1, 2, 150, 1500100, 1001));
    
    EXPECT_EQ(book.total_orders(), 1);
    EXPECT_EQ(*book.best_bid(), 1500100);
    EXPECT_EQ(*book.best_bid_size(), 150);
}

TEST_F(EnhancedOrderBookTest, MultipleOrdersSamePrice) {
    book.add_order(1, 'B', 1500000, 100, 1000);
    book.add_order(2, 'B', 1500000, 200, 1001);
    
    EXPECT_EQ(*book.best_bid_size(), 300);
    EXPECT_EQ(book.total_orders(), 2);
}

TEST_F(EnhancedOrderBookTest, OrderBookDepth) {
    book.add_order(1, 'B', 1500000, 100, 1000);
    book.add_order(2, 'B', 1499900, 200, 1001);
    book.add_order(3, 'B', 1499800, 150, 1002);
    
    auto depth = book.get_bid_depth(2);
    EXPECT_EQ(depth.size(), 2);
    EXPECT_EQ(depth[0].price, 1500000);
    EXPECT_EQ(depth[1].price, 1499900);
}

TEST_F(EnhancedOrderBookTest, CrossingOrders) {
    book.add_bid(1500100, 100, 1000);
    book.add_ask(1500000, 100, 1001);
    
    EXPECT_TRUE(book.has_crossing());
}

TEST_F(EnhancedOrderBookTest, NoCrossing) {
    book.add_bid(1500000, 100, 1000);
    book.add_ask(1500100, 100, 1001);
    
    EXPECT_FALSE(book.has_crossing());
}

TEST_F(EnhancedOrderBookTest, MidPrice) {
    book.add_bid(1500000, 100, 1000);
    book.add_ask(1500100, 100, 1001);
    
    double expected_mid = (1500000 + 1500100) / 20000.0;
    EXPECT_DOUBLE_EQ(book.mid_price(), expected_mid);
}

TEST_F(EnhancedOrderBookTest, StressTest) {
    const size_t num_orders = 1000;
    
    for (size_t i = 0; i < num_orders; ++i) {
        int64_t price = 1500000 + (i % 100) * 100;
        char side = (i % 2 == 0) ? 'B' : 'S';
        EXPECT_TRUE(book.add_order(i, side, price, 100, i));
    }
    
    EXPECT_EQ(book.total_orders(), num_orders);
    
    for (size_t i = 0; i < num_orders; ++i) {
        if (i % 3 == 0) {
            EXPECT_TRUE(book.delete_order(i, num_orders + i));
        }
    }
    
    EXPECT_LT(book.total_orders(), num_orders);
}
