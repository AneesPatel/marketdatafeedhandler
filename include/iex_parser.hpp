#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <optional>
#include <cstring>

namespace iex {

enum class MessageType : uint8_t {
    SystemEvent = 0x53,
    SecurityDirectory = 0x44,
    TradingStatus = 0x48,
    OperationalHalt = 0x4F,
    ShortSalePriceTest = 0x50,
    QuoteUpdate = 0x51,
    TradeReport = 0x54,
    OfficialPrice = 0x58,
    TradeBreak = 0x42,
    AuctionInformation = 0x41,
    PriceLevelUpdate = 0x38
};

#pragma pack(push, 1)

struct MessageHeader {
    uint8_t type;
    uint64_t timestamp;
    
    MessageType get_type() const {
        return static_cast<MessageType>(type);
    }
};

struct SystemEvent {
    MessageHeader header;
    uint8_t event;
};

struct SecurityDirectory {
    MessageHeader header;
    uint8_t flags;
    uint64_t timestamp;
    char symbol[8];
    uint32_t round_lot;
    uint64_t adjusted_poc_close;
    uint8_t luld_tier;
};

struct TradingStatus {
    MessageHeader header;
    uint8_t status;
    char symbol[8];
    char reason[4];
};

struct QuoteUpdate {
    MessageHeader header;
    uint8_t flags;
    char symbol[8];
    uint32_t bid_size;
    int64_t bid_price;
    uint32_t ask_size;
    int64_t ask_price;
};

struct TradeReport {
    MessageHeader header;
    uint8_t flags;
    char symbol[8];
    uint32_t size;
    int64_t price;
    uint64_t trade_id;
};

struct PriceLevelUpdate {
    MessageHeader header;
    uint8_t flags;
    char symbol[8];
    uint32_t size;
    int64_t price;
};

#pragma pack(pop)

using Message = std::variant<
    SystemEvent,
    SecurityDirectory,
    TradingStatus,
    QuoteUpdate,
    TradeReport,
    PriceLevelUpdate
>;

class Parser {
    const uint8_t* buffer_;
    size_t size_;
    size_t offset_;
    
public:
    Parser(const uint8_t* buffer, size_t size)
        : buffer_(buffer), size_(size), offset_(0) {}
    
    std::optional<Message> parse_next() {
        if (offset_ + sizeof(MessageHeader) > size_) {
            return std::nullopt;
        }
        
        const MessageHeader* header = 
            reinterpret_cast<const MessageHeader*>(buffer_ + offset_);
        
        MessageType type = header->get_type();
        
        switch (type) {
            case MessageType::SystemEvent:
                return parse_message<SystemEvent>();
            case MessageType::SecurityDirectory:
                return parse_message<SecurityDirectory>();
            case MessageType::TradingStatus:
                return parse_message<TradingStatus>();
            case MessageType::QuoteUpdate:
                return parse_message<QuoteUpdate>();
            case MessageType::TradeReport:
                return parse_message<TradeReport>();
            case MessageType::PriceLevelUpdate:
                return parse_message<PriceLevelUpdate>();
            default:
                offset_ += sizeof(MessageHeader);
                return std::nullopt;
        }
    }
    
    template<typename T>
    std::optional<Message> parse_message() {
        if (offset_ + sizeof(T) > size_) {
            return std::nullopt;
        }
        
        T msg;
        std::memcpy(&msg, buffer_ + offset_, sizeof(T));
        offset_ += sizeof(T);
        
        return Message(msg);
    }
    
    bool has_more() const {
        return offset_ < size_;
    }
    
    void reset() {
        offset_ = 0;
    }
    
    size_t position() const {
        return offset_;
    }
};

inline std::string symbol_to_string(const char* symbol, size_t len = 8) {
    size_t actual_len = 0;
    for (size_t i = 0; i < len; ++i) {
        if (symbol[i] == ' ' || symbol[i] == '\0') break;
        actual_len++;
    }
    return std::string(symbol, actual_len);
}

inline int64_t price_to_double_scale(int64_t price) {
    return price;
}

inline double price_to_double(int64_t price) {
    return price / 10000.0;
}

}
