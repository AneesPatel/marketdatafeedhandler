#include "marketdata_parser.hpp"
#include <cstring>

namespace marketdata {

static inline uint16_t swap16(uint16_t val) {
    return (val >> 8) | (val << 8);
}

static inline uint32_t swap32(uint32_t val) {
    return ((val >> 24) & 0x000000FF) |
           ((val >> 8)  & 0x0000FF00) |
           ((val << 8)  & 0x00FF0000) |
           ((val << 24) & 0xFF000000);
}

static inline uint64_t swap64(uint64_t val) {
    return ((val >> 56) & 0x00000000000000FF) |
           ((val >> 40) & 0x000000000000FF00) |
           ((val >> 24) & 0x0000000000FF0000) |
           ((val >> 8)  & 0x00000000FF000000) |
           ((val << 8)  & 0x000000FF00000000) |
           ((val << 24) & 0x0000FF0000000000) |
           ((val << 40) & 0x00FF000000000000) |
           ((val << 56) & 0xFF00000000000000);
}

static inline uint64_t read_timestamp(const uint8_t* p) {
    uint64_t ts = 0;
    ts |= static_cast<uint64_t>(p[0]) << 40;
    ts |= static_cast<uint64_t>(p[1]) << 32;
    ts |= static_cast<uint64_t>(p[2]) << 24;
    ts |= static_cast<uint64_t>(p[3]) << 16;
    ts |= static_cast<uint64_t>(p[4]) << 8;
    ts |= static_cast<uint64_t>(p[5]);
    return ts;
}

bool ITCHParser::parse_add_order(const uint8_t* data, size_t len, Order& out) {
    if (len < 36) return false;
    if (data[0] != 'A') return false;
    
    out.timestamp = read_timestamp(data + 5);
    
    uint64_t ref;
    std::memcpy(&ref, data + 11, 8);
    out.order_ref = swap64(ref);
    
    out.is_buy = (data[19] == 'B');
    
    uint32_t shares;
    std::memcpy(&shares, data + 20, 4);
    out.shares = swap32(shares);
    
    std::memcpy(out.symbol, data + 24, 8);
    out.symbol[8] = '\0';
    
    uint32_t price;
    std::memcpy(&price, data + 32, 4);
    out.price = swap32(price);
    
    return true;
}

}
