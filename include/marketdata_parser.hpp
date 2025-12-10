#pragma once
#include <cstdint>
#include <cstddef>

namespace marketdata {

struct Order {
    uint64_t timestamp;
    uint64_t order_ref;
    uint32_t shares;
    uint32_t price;
    char symbol[9];
    bool is_buy;
};

class ITCHParser {
public:
    static bool parse_add_order(const uint8_t* data, size_t len, Order& out);
};

}
