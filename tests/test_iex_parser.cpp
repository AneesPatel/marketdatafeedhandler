#include <gtest/gtest.h>
#include "iex_parser.hpp"

class IEXParserTest : public ::testing::Test {
protected:
    std::vector<uint8_t> create_quote_update(const char* symbol, 
                                             int64_t bid_price, uint32_t bid_size,
                                             int64_t ask_price, uint32_t ask_size) {
        iex::QuoteUpdate quote{};
        quote.header.type = static_cast<uint8_t>(iex::MessageType::QuoteUpdate);
        quote.header.timestamp = 1000000;
        quote.flags = 0;
        std::memcpy(quote.symbol, symbol, std::min(strlen(symbol), size_t(8)));
        quote.bid_price = bid_price;
        quote.bid_size = bid_size;
        quote.ask_price = ask_price;
        quote.ask_size = ask_size;
        
        std::vector<uint8_t> buffer(sizeof(quote));
        std::memcpy(buffer.data(), &quote, sizeof(quote));
        return buffer;
    }
};

TEST_F(IEXParserTest, ParseQuoteUpdate) {
    auto buffer = create_quote_update("AAPL    ", 1500000, 100, 1500100, 200);
    
    iex::Parser parser(buffer.data(), buffer.size());
    
    ASSERT_TRUE(parser.has_more());
    
    auto msg = parser.parse_next();
    ASSERT_TRUE(msg.has_value());
    
    ASSERT_TRUE(std::holds_alternative<iex::QuoteUpdate>(*msg));
    
    auto& quote = std::get<iex::QuoteUpdate>(*msg);
    EXPECT_EQ(quote.bid_price, 1500000);
    EXPECT_EQ(quote.bid_size, 100);
    EXPECT_EQ(quote.ask_price, 1500100);
    EXPECT_EQ(quote.ask_size, 200);
    
    std::string symbol = iex::symbol_to_string(quote.symbol);
    EXPECT_EQ(symbol, "AAPL");
}

TEST_F(IEXParserTest, ParseMultipleMessages) {
    std::vector<uint8_t> buffer;
    
    auto msg1 = create_quote_update("AAPL    ", 1500000, 100, 1500100, 200);
    auto msg2 = create_quote_update("MSFT    ", 3000000, 150, 3000050, 250);
    
    buffer.insert(buffer.end(), msg1.begin(), msg1.end());
    buffer.insert(buffer.end(), msg2.begin(), msg2.end());
    
    iex::Parser parser(buffer.data(), buffer.size());
    
    auto parsed1 = parser.parse_next();
    ASSERT_TRUE(parsed1.has_value());
    auto& quote1 = std::get<iex::QuoteUpdate>(*parsed1);
    EXPECT_EQ(iex::symbol_to_string(quote1.symbol), "AAPL");
    
    auto parsed2 = parser.parse_next();
    ASSERT_TRUE(parsed2.has_value());
    auto& quote2 = std::get<iex::QuoteUpdate>(*parsed2);
    EXPECT_EQ(iex::symbol_to_string(quote2.symbol), "MSFT");
    
    EXPECT_FALSE(parser.has_more());
}

TEST_F(IEXParserTest, PriceConversion) {
    int64_t price = 1500000;
    double expected = 150.0;
    
    EXPECT_DOUBLE_EQ(iex::price_to_double(price), expected);
}

TEST_F(IEXParserTest, EmptyBuffer) {
    std::vector<uint8_t> buffer;
    iex::Parser parser(buffer.data(), buffer.size());
    
    EXPECT_FALSE(parser.has_more());
    EXPECT_FALSE(parser.parse_next().has_value());
}

TEST_F(IEXParserTest, PartialMessage) {
    auto buffer = create_quote_update("AAPL    ", 1500000, 100, 1500100, 200);
    buffer.resize(10);
    
    iex::Parser parser(buffer.data(), buffer.size());
    
    auto msg = parser.parse_next();
    EXPECT_FALSE(msg.has_value());
}
