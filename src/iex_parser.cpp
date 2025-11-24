#include "iex_parser.hpp"
#include <cstring>
#include <algorithm>

namespace iex {

std::string symbol_to_string(const char symbol[8]) {
    std::string result(symbol, 8);
    result.erase(std::find_if(result.rbegin(), result.rend(), 
                 [](unsigned char ch) { return !std::isspace(ch); }).base(), 
                 result.end());
    return result;
}

Parser::Parser(const uint8_t* data, size_t size) 
    : data_(data), size_(size), offset_(0) {}

bool Parser::has_more() const {
    return offset_ < size_;
}

std::optional<Message> Parser::parse_next() {
    if (!has_more() || size_ - offset_ < sizeof(MessageHeader)) {
        return std::nullopt;
    }

    const MessageHeader* header = reinterpret_cast<const MessageHeader*>(data_ + offset_);
    MessageType type = static_cast<MessageType>(header->type);

    std::optional<Message> result;

    switch (type) {
        case MessageType::QuoteUpdate: {
            if (size_ - offset_ >= sizeof(QuoteUpdate)) {
                QuoteUpdate quote;
                std::memcpy(&quote, data_ + offset_, sizeof(QuoteUpdate));
                result = quote;
                offset_ += sizeof(QuoteUpdate);
            }
            break;
        }
        case MessageType::TradeReport: {
            if (size_ - offset_ >= sizeof(TradeReport)) {
                TradeReport trade;
                std::memcpy(&trade, data_ + offset_, sizeof(TradeReport));
                result = trade;
                offset_ += sizeof(TradeReport);
            }
            break;
        }
        case MessageType::PriceLevelUpdate: {
            if (size_ - offset_ >= sizeof(PriceLevelUpdate)) {
                PriceLevelUpdate update;
                std::memcpy(&update, data_ + offset_, sizeof(PriceLevelUpdate));
                result = update;
                offset_ += sizeof(PriceLevelUpdate);
            }
            break;
        }
        case MessageType::TradeBreak: {
            if (size_ - offset_ >= sizeof(TradeBreak)) {
                TradeBreak tb;
                std::memcpy(&tb, data_ + offset_, sizeof(TradeBreak));
                result = tb;
                offset_ += sizeof(TradeBreak);
            }
            break;
        }
        case MessageType::AuctionInformation: {
            if (size_ - offset_ >= sizeof(AuctionInformation)) {
                AuctionInformation auction;
                std::memcpy(&auction, data_ + offset_, sizeof(AuctionInformation));
                result = auction;
                offset_ += sizeof(AuctionInformation);
            }
            break;
        }
        case MessageType::SecurityDirectory: {
            if (size_ - offset_ >= sizeof(SecurityDirectory)) {
                SecurityDirectory dir;
                std::memcpy(&dir, data_ + offset_, sizeof(SecurityDirectory));
                result = dir;
                offset_ += sizeof(SecurityDirectory);
            }
            break;
        }
        case MessageType::TradingStatus: {
            if (size_ - offset_ >= sizeof(TradingStatus)) {
                TradingStatus status;
                std::memcpy(&status, data_ + offset_, sizeof(TradingStatus));
                result = status;
                offset_ += sizeof(TradingStatus);
            }
            break;
        }
        case MessageType::SystemEvent: {
            if (size_ - offset_ >= sizeof(SystemEvent)) {
                SystemEvent event;
                std::memcpy(&event, data_ + offset_, sizeof(SystemEvent));
                result = event;
                offset_ += sizeof(SystemEvent);
            }
            break;
        }
        default:
            offset_ += sizeof(MessageHeader);
            break;
    }

    return result;
}

}
