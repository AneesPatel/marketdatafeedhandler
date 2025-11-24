#include <gtest/gtest.h>
#include "itch_parser.hpp"

class ITCHParserTest : public ::testing::Test {
protected:
    std::vector<uint8_t> create_add_order(uint64_t order_ref, char side, 
                                          uint32_t shares, const char* stock,
                                          uint32_t price) {
        itch::AddOrder order{};
        order.length = sizeof(itch::AddOrder);
        order.type = 'A';
        order.stock_locate = 0;
        order.tracking_number = 1;
        order.timestamp = 1000000;
        order.order_reference = order_ref;
        order.buy_sell = side;
        order.shares = shares;
        std::memcpy(order.stock, stock, std::min(strlen(stock), size_t(8)));
        order.price = price;
        
        order.length = itch::swap_uint16(order.length);
        order.stock_locate = itch::swap_uint16(order.stock_locate);
        order.tracking_number = itch::swap_uint16(order.tracking_number);
        order.timestamp = itch::swap_uint64(order.timestamp);
        order.order_reference = itch::swap_uint64(order_ref);
        order.shares = itch::swap_uint32(shares);
        order.price = itch::swap_uint32(price);
        
        std::vector<uint8_t> buffer(sizeof(order));
        std::memcpy(buffer.data(), &order, sizeof(order));
        return buffer;
    }
};

TEST_F(ITCHParserTest, ParseAddOrder) {
    auto buffer = create_add_order(12345, 'B', 100, "AAPL    ", 1500000);
    
    itch::Parser parser(buffer.data(), buffer.size());
    
    ASSERT_TRUE(parser.has_more());
    
    auto msg = parser.parse_next();
    ASSERT_TRUE(msg.has_value());
    
    ASSERT_TRUE(std::holds_alternative<itch::AddOrder>(*msg));
    
    auto& order = std::get<itch::AddOrder>(*msg);
    EXPECT_EQ(order.order_reference, 12345);
    EXPECT_EQ(order.buy_sell, 'B');
    EXPECT_EQ(order.shares, 100);
    EXPECT_EQ(order.price, 1500000);
    
    std::string stock = itch::stock_to_string(order.stock);
    EXPECT_EQ(stock, "AAPL");
}

TEST_F(ITCHParserTest, EndiannessConversion) {
    uint16_t val16 = 0x1234;
    EXPECT_EQ(itch::swap_uint16(val16), 0x3412);
    
    uint32_t val32 = 0x12345678;
    EXPECT_EQ(itch::swap_uint32(val32), 0x78563412);
    
    uint64_t val64 = 0x123456789ABCDEF0;
    EXPECT_EQ(itch::swap_uint64(val64), 0xF0DEBC9A78563412);
}

TEST_F(ITCHParserTest, PriceConversion) {
    uint32_t price = 1500000;
    double expected = 150.0;
    
    EXPECT_DOUBLE_EQ(itch::price_to_double(price), expected);
}
