#include "itch_parser.hpp"
#include <cstring>
#include <algorithm>

namespace itch {

uint16_t swap_uint16(uint16_t val) {
    return (val >> 8) | (val << 8);
}

uint32_t swap_uint32(uint32_t val) {
    return ((val >> 24) & 0xff) | 
           ((val >> 8) & 0xff00) | 
           ((val << 8) & 0xff0000) | 
           ((val << 24) & 0xff000000);
}

uint64_t swap_uint64(uint64_t val) {
    return ((val >> 56) & 0x00000000000000ffULL) |
           ((val >> 40) & 0x000000000000ff00ULL) |
           ((val >> 24) & 0x0000000000ff0000ULL) |
           ((val >> 8)  & 0x00000000ff000000ULL) |
           ((val << 8)  & 0x000000ff00000000ULL) |
           ((val << 24) & 0x0000ff0000000000ULL) |
           ((val << 40) & 0x00ff000000000000ULL) |
           ((val << 56) & 0xff00000000000000ULL);
}

std::string stock_to_string(const char stock[8]) {
    std::string result(stock, 8);
    result.erase(std::find_if(result.rbegin(), result.rend(), 
                 [](unsigned char ch) { return !std::isspace(ch); }).base(), 
                 result.end());
    return result;
}

Parser::Parser(const uint8_t* data, size_t size) 
    : buffer_(data), size_(size), offset_(0) {}

bool Parser::has_more() const {
    return offset_ < size_;
}

std::optional<Message> Parser::parse_next() {
    if (offset_ + sizeof(MessageHeader) > size_) {
        return std::nullopt;
    }
    
    const MessageHeader* header = 
        reinterpret_cast<const MessageHeader*>(buffer_ + offset_);
    
    uint16_t msg_length = swap_uint16(header->length);
    uint8_t msg_type = header->type;
    
    if (offset_ + msg_length > size_) {
        return std::nullopt;
    }
    
    Message result;
    bool parsed = false;
    
    switch (msg_type) {
        case 'S': {
            SystemEvent msg;
            std::memcpy(&msg, buffer_ + offset_, sizeof(msg));
            convert_endianness(msg);
            result = msg;
            parsed = true;
            break;
        }
        case 'R': {
            StockDirectory msg;
            std::memcpy(&msg, buffer_ + offset_, sizeof(msg));
            convert_endianness(msg);
            result = msg;
            parsed = true;
            break;
        }
        case 'A': {
            AddOrder msg;
            std::memcpy(&msg, buffer_ + offset_, sizeof(msg));
            convert_endianness(msg);
            result = msg;
            parsed = true;
            break;
        }
        case 'F': {
            AddOrderMPID msg;
            std::memcpy(&msg, buffer_ + offset_, sizeof(msg));
            convert_endianness(msg);
            result = msg;
            parsed = true;
            break;
        }
        case 'E': {
            OrderExecuted msg;
            std::memcpy(&msg, buffer_ + offset_, sizeof(msg));
            convert_endianness(msg);
            result = msg;
            parsed = true;
            break;
        }
        case 'C': {
            OrderExecutedWithPrice msg;
            std::memcpy(&msg, buffer_ + offset_, sizeof(msg));
            convert_endianness(msg);
            result = msg;
            parsed = true;
            break;
        }
        case 'X': {
            OrderCancel msg;
            std::memcpy(&msg, buffer_ + offset_, sizeof(msg));
            convert_endianness(msg);
            result = msg;
            parsed = true;
            break;
        }
        case 'D': {
            OrderDelete msg;
            std::memcpy(&msg, buffer_ + offset_, sizeof(msg));
            convert_endianness(msg);
            result = msg;
            parsed = true;
            break;
        }
        case 'U': {
            OrderReplace msg;
            std::memcpy(&msg, buffer_ + offset_, sizeof(msg));
            convert_endianness(msg);
            result = msg;
            parsed = true;
            break;
        }
        case 'P': {
            Trade msg;
            std::memcpy(&msg, buffer_ + offset_, sizeof(msg));
            convert_endianness(msg);
            result = msg;
            parsed = true;
            break;
        }
        default:
            offset_ += msg_length;
            return std::nullopt;
    }
    
    offset_ += msg_length;
    
    return parsed ? std::optional<Message>(result) : std::nullopt;
}

template<>
void Parser::convert_endianness(SystemEvent& msg) {
    msg.length = swap_uint16(msg.length);
    msg.stock_locate = swap_uint16(msg.stock_locate);
    msg.tracking_number = swap_uint16(msg.tracking_number);
    msg.timestamp = swap_uint64(msg.timestamp);
}

template<>
void Parser::convert_endianness(StockDirectory& msg) {
    msg.length = swap_uint16(msg.length);
    msg.stock_locate = swap_uint16(msg.stock_locate);
    msg.tracking_number = swap_uint16(msg.tracking_number);
    msg.timestamp = swap_uint64(msg.timestamp);
    msg.round_lot_size = swap_uint32(msg.round_lot_size);
    msg.etp_leverage_factor = swap_uint32(msg.etp_leverage_factor);
}

template<>
void Parser::convert_endianness(AddOrder& msg) {
    msg.length = swap_uint16(msg.length);
    msg.stock_locate = swap_uint16(msg.stock_locate);
    msg.tracking_number = swap_uint16(msg.tracking_number);
    msg.timestamp = swap_uint64(msg.timestamp);
    msg.order_reference = swap_uint64(msg.order_reference);
    msg.shares = swap_uint32(msg.shares);
    msg.price = swap_uint32(msg.price);
}

template<>
void Parser::convert_endianness(AddOrderMPID& msg) {
    msg.length = swap_uint16(msg.length);
    msg.stock_locate = swap_uint16(msg.stock_locate);
    msg.tracking_number = swap_uint16(msg.tracking_number);
    msg.timestamp = swap_uint64(msg.timestamp);
    msg.order_reference = swap_uint64(msg.order_reference);
    msg.shares = swap_uint32(msg.shares);
    msg.price = swap_uint32(msg.price);
}

template<>
void Parser::convert_endianness(OrderExecuted& msg) {
    msg.length = swap_uint16(msg.length);
    msg.stock_locate = swap_uint16(msg.stock_locate);
    msg.tracking_number = swap_uint16(msg.tracking_number);
    msg.timestamp = swap_uint64(msg.timestamp);
    msg.order_reference = swap_uint64(msg.order_reference);
    msg.executed_shares = swap_uint32(msg.executed_shares);
    msg.match_number = swap_uint64(msg.match_number);
}

template<>
void Parser::convert_endianness(OrderExecutedWithPrice& msg) {
    msg.length = swap_uint16(msg.length);
    msg.stock_locate = swap_uint16(msg.stock_locate);
    msg.tracking_number = swap_uint16(msg.tracking_number);
    msg.timestamp = swap_uint64(msg.timestamp);
    msg.order_reference = swap_uint64(msg.order_reference);
    msg.executed_shares = swap_uint32(msg.executed_shares);
    msg.match_number = swap_uint64(msg.match_number);
    msg.execution_price = swap_uint32(msg.execution_price);
}

template<>
void Parser::convert_endianness(OrderCancel& msg) {
    msg.length = swap_uint16(msg.length);
    msg.stock_locate = swap_uint16(msg.stock_locate);
    msg.tracking_number = swap_uint16(msg.tracking_number);
    msg.timestamp = swap_uint64(msg.timestamp);
    msg.order_reference = swap_uint64(msg.order_reference);
    msg.cancelled_shares = swap_uint32(msg.cancelled_shares);
}

template<>
void Parser::convert_endianness(OrderDelete& msg) {
    msg.length = swap_uint16(msg.length);
    msg.stock_locate = swap_uint16(msg.stock_locate);
    msg.tracking_number = swap_uint16(msg.tracking_number);
    msg.timestamp = swap_uint64(msg.timestamp);
    msg.order_reference = swap_uint64(msg.order_reference);
}

template<>
void Parser::convert_endianness(OrderReplace& msg) {
    msg.length = swap_uint16(msg.length);
    msg.stock_locate = swap_uint16(msg.stock_locate);
    msg.tracking_number = swap_uint16(msg.tracking_number);
    msg.timestamp = swap_uint64(msg.timestamp);
    msg.original_order_reference = swap_uint64(msg.original_order_reference);
    msg.new_order_reference = swap_uint64(msg.new_order_reference);
    msg.shares = swap_uint32(msg.shares);
    msg.price = swap_uint32(msg.price);
}

template<>
void Parser::convert_endianness(Trade& msg) {
    msg.length = swap_uint16(msg.length);
    msg.stock_locate = swap_uint16(msg.stock_locate);
    msg.tracking_number = swap_uint16(msg.tracking_number);
    msg.timestamp = swap_uint64(msg.timestamp);
    msg.order_reference = swap_uint64(msg.order_reference);
    msg.shares = swap_uint32(msg.shares);
    msg.price = swap_uint32(msg.price);
    msg.match_number = swap_uint64(msg.match_number);
}

}
