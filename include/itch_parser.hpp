#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <optional>
#include <array>

namespace itch {

enum class MessageType : uint8_t {
    SystemEvent = 'S',
    StockDirectory = 'R',
    StockTradingAction = 'H',
    RegSHORestriction = 'Y',
    MarketParticipantPosition = 'L',
    MWCBDeclineLevel = 'V',
    MWCBStatus = 'W',
    IPOQuotingPeriod = 'K',
    AddOrder = 'A',
    AddOrderMPID = 'F',
    OrderExecuted = 'E',
    OrderExecutedWithPrice = 'C',
    OrderCancel = 'X',
    OrderDelete = 'D',
    OrderReplace = 'U',
    Trade = 'P',
    CrossTrade = 'Q',
    BrokenTrade = 'B',
    NOII = 'I'
};

#pragma pack(push, 1)

struct MessageHeader {
    uint16_t length;
    uint8_t type;
};

struct SystemEvent {
    uint16_t length;
    uint8_t type;
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    uint8_t event_code;
};

struct StockDirectory {
    uint16_t length;
    uint8_t type;
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    char stock[8];
    char market_category;
    char financial_status;
    uint32_t round_lot_size;
    char round_lots_only;
    char issue_classification;
    char issue_sub_type[2];
    char authenticity;
    char short_sale_threshold;
    char ipo_flag;
    char luld_reference_price_tier;
    char etp_flag;
    uint32_t etp_leverage_factor;
    char inverse_indicator;
};

struct AddOrder {
    uint16_t length;
    uint8_t type;
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    uint64_t order_reference;
    char buy_sell;
    uint32_t shares;
    char stock[8];
    uint32_t price;
};

struct AddOrderMPID {
    uint16_t length;
    uint8_t type;
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    uint64_t order_reference;
    char buy_sell;
    uint32_t shares;
    char stock[8];
    uint32_t price;
    char attribution[4];
};

struct OrderExecuted {
    uint16_t length;
    uint8_t type;
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    uint64_t order_reference;
    uint32_t executed_shares;
    uint64_t match_number;
};

struct OrderExecutedWithPrice {
    uint16_t length;
    uint8_t type;
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    uint64_t order_reference;
    uint32_t executed_shares;
    uint64_t match_number;
    char printable;
    uint32_t execution_price;
};

struct OrderCancel {
    uint16_t length;
    uint8_t type;
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    uint64_t order_reference;
    uint32_t cancelled_shares;
};

struct OrderDelete {
    uint16_t length;
    uint8_t type;
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    uint64_t order_reference;
};

struct OrderReplace {
    uint16_t length;
    uint8_t type;
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    uint64_t original_order_reference;
    uint64_t new_order_reference;
    uint32_t shares;
    uint32_t price;
};

struct Trade {
    uint16_t length;
    uint8_t type;
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    uint64_t order_reference;
    char buy_sell;
    uint32_t shares;
    char stock[8];
    uint32_t price;
    uint64_t match_number;
};

#pragma pack(pop)

using Message = std::variant<
    SystemEvent,
    StockDirectory,
    AddOrder,
    AddOrderMPID,
    OrderExecuted,
    OrderExecutedWithPrice,
    OrderCancel,
    OrderDelete,
    OrderReplace,
    Trade
>;

template<uint8_t MsgType>
struct MessageParser;

template<>
struct MessageParser<'S'> {
    using type = SystemEvent;
    static constexpr size_t size = sizeof(SystemEvent);
};

template<>
struct MessageParser<'R'> {
    using type = StockDirectory;
    static constexpr size_t size = sizeof(StockDirectory);
};

template<>
struct MessageParser<'A'> {
    using type = AddOrder;
    static constexpr size_t size = sizeof(AddOrder);
};

template<>
struct MessageParser<'F'> {
    using type = AddOrderMPID;
    static constexpr size_t size = sizeof(AddOrderMPID);
};

template<>
struct MessageParser<'E'> {
    using type = OrderExecuted;
    static constexpr size_t size = sizeof(OrderExecuted);
};

template<>
struct MessageParser<'C'> {
    using type = OrderExecutedWithPrice;
    static constexpr size_t size = sizeof(OrderExecutedWithPrice);
};

template<>
struct MessageParser<'X'> {
    using type = OrderCancel;
    static constexpr size_t size = sizeof(OrderCancel);
};

template<>
struct MessageParser<'D'> {
    using type = OrderDelete;
    static constexpr size_t size = sizeof(OrderDelete);
};

template<>
struct MessageParser<'U'> {
    using type = OrderReplace;
    static constexpr size_t size = sizeof(OrderReplace);
};

template<>
struct MessageParser<'P'> {
    using type = Trade;
    static constexpr size_t size = sizeof(Trade);
};

inline uint16_t swap_uint16(uint16_t val) {
    return (val << 8) | (val >> 8);
}

inline uint32_t swap_uint32(uint32_t val) {
    return ((val & 0xFF000000) >> 24) |
           ((val & 0x00FF0000) >> 8) |
           ((val & 0x0000FF00) << 8) |
           ((val & 0x000000FF) << 24);
}

inline uint64_t swap_uint64(uint64_t val) {
    return ((val & 0xFF00000000000000ULL) >> 56) |
           ((val & 0x00FF000000000000ULL) >> 40) |
           ((val & 0x0000FF0000000000ULL) >> 24) |
           ((val & 0x000000FF00000000ULL) >> 8) |
           ((val & 0x00000000FF000000ULL) << 8) |
           ((val & 0x0000000000FF0000ULL) << 24) |
           ((val & 0x000000000000FF00ULL) << 40) |
           ((val & 0x00000000000000FFULL) << 56);
}

class Parser {
    const uint8_t* buffer_;
    size_t size_;
    size_t offset_;
    
public:
    Parser(const uint8_t* buffer, size_t size)
        : buffer_(buffer), size_(size), offset_(0) {}
    
    std::optional<Message> parse_next();
    
    template<typename T>
    void convert_endianness(T& msg);
    
    bool has_more() const { return offset_ < size_; }
    void reset() { offset_ = 0; }
    size_t position() const { return offset_; }
};

inline std::string stock_to_string(const char* stock, size_t len = 8) {
    size_t actual_len = 0;
    for (size_t i = 0; i < len; ++i) {
        if (stock[i] == ' ' || stock[i] == '\0') break;
        actual_len++;
    }
    return std::string(stock, actual_len);
}

inline double price_to_double(uint32_t price) {
    return price / 10000.0;
}

}
