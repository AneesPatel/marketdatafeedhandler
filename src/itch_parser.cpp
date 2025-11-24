#include "itch_parser.hpp"
#include <cstring>
#include <algorithm>

namespace itch {

std::optional<Message> Parser::parse_next() {
    if (offset_ + 3 > size_) {
        return std::nullopt;
    }

    uint16_t msg_length = swap_uint16(*reinterpret_cast<const uint16_t*>(buffer_ + offset_));
    char msg_type = static_cast<char>(buffer_[offset_ + 2]);

    if (offset_ + msg_length > size_) {
        return std::nullopt;
    }

    std::optional<Message> result;

    switch (msg_type) {
        case 'S': {
            if (msg_length >= sizeof(SystemEvent)) {
                SystemEvent event;
                std::memcpy(&event, buffer_ + offset_, sizeof(SystemEvent));
                event.length = swap_uint16(event.length);
                event.stock_locate = swap_uint16(event.stock_locate);
                event.tracking_number = swap_uint16(event.tracking_number);
                event.timestamp = swap_uint64(event.timestamp);
                result = event;
            }
            break;
        }
        case 'R': {
            if (msg_length >= sizeof(StockDirectory)) {
                StockDirectory dir;
                std::memcpy(&dir, buffer_ + offset_, sizeof(StockDirectory));
                dir.length = swap_uint16(dir.length);
                dir.stock_locate = swap_uint16(dir.stock_locate);
                dir.tracking_number = swap_uint16(dir.tracking_number);
                dir.timestamp = swap_uint64(dir.timestamp);
                dir.round_lot_size = swap_uint32(dir.round_lot_size);
                dir.etp_leverage_factor = swap_uint32(dir.etp_leverage_factor);
                result = dir;
            }
            break;
        }
        case 'A': {
            if (msg_length >= sizeof(AddOrder)) {
                AddOrder add;
                std::memcpy(&add, buffer_ + offset_, sizeof(AddOrder));
                add.length = swap_uint16(add.length);
                add.stock_locate = swap_uint16(add.stock_locate);
                add.tracking_number = swap_uint16(add.tracking_number);
                add.timestamp = swap_uint64(add.timestamp);
                add.order_reference = swap_uint64(add.order_reference);
                add.shares = swap_uint32(add.shares);
                add.price = swap_uint32(add.price);
                result = add;
            }
            break;
        }
        case 'F': {
            if (msg_length >= sizeof(AddOrderMPID)) {
                AddOrderMPID add;
                std::memcpy(&add, buffer_ + offset_, sizeof(AddOrderMPID));
                add.length = swap_uint16(add.length);
                add.stock_locate = swap_uint16(add.stock_locate);
                add.tracking_number = swap_uint16(add.tracking_number);
                add.timestamp = swap_uint64(add.timestamp);
                add.order_reference = swap_uint64(add.order_reference);
                add.shares = swap_uint32(add.shares);
                add.price = swap_uint32(add.price);
                result = add;
            }
            break;
        }
        case 'E': {
            if (msg_length >= sizeof(OrderExecuted)) {
                OrderExecuted exec;
                std::memcpy(&exec, buffer_ + offset_, sizeof(OrderExecuted));
                exec.length = swap_uint16(exec.length);
                exec.stock_locate = swap_uint16(exec.stock_locate);
                exec.tracking_number = swap_uint16(exec.tracking_number);
                exec.timestamp = swap_uint64(exec.timestamp);
                exec.order_reference = swap_uint64(exec.order_reference);
                exec.executed_shares = swap_uint32(exec.executed_shares);
                exec.match_number = swap_uint64(exec.match_number);
                result = exec;
            }
            break;
        }
        case 'C': {
            if (msg_length >= sizeof(OrderExecutedWithPrice)) {
                OrderExecutedWithPrice exec;
                std::memcpy(&exec, buffer_ + offset_, sizeof(OrderExecutedWithPrice));
                exec.length = swap_uint16(exec.length);
                exec.stock_locate = swap_uint16(exec.stock_locate);
                exec.tracking_number = swap_uint16(exec.tracking_number);
                exec.timestamp = swap_uint64(exec.timestamp);
                exec.order_reference = swap_uint64(exec.order_reference);
                exec.executed_shares = swap_uint32(exec.executed_shares);
                exec.match_number = swap_uint64(exec.match_number);
                exec.execution_price = swap_uint32(exec.execution_price);
                result = exec;
            }
            break;
        }
        case 'X': {
            if (msg_length >= sizeof(OrderCancel)) {
                OrderCancel cancel;
                std::memcpy(&cancel, buffer_ + offset_, sizeof(OrderCancel));
                cancel.length = swap_uint16(cancel.length);
                cancel.stock_locate = swap_uint16(cancel.stock_locate);
                cancel.tracking_number = swap_uint16(cancel.tracking_number);
                cancel.timestamp = swap_uint64(cancel.timestamp);
                cancel.order_reference = swap_uint64(cancel.order_reference);
                cancel.cancelled_shares = swap_uint32(cancel.cancelled_shares);
                result = cancel;
            }
            break;
        }
        case 'D': {
            if (msg_length >= sizeof(OrderDelete)) {
                OrderDelete del;
                std::memcpy(&del, buffer_ + offset_, sizeof(OrderDelete));
                del.length = swap_uint16(del.length);
                del.stock_locate = swap_uint16(del.stock_locate);
                del.tracking_number = swap_uint16(del.tracking_number);
                del.timestamp = swap_uint64(del.timestamp);
                del.order_reference = swap_uint64(del.order_reference);
                result = del;
            }
            break;
        }
        case 'U': {
            if (msg_length >= sizeof(OrderReplace)) {
                OrderReplace replace;
                std::memcpy(&replace, buffer_ + offset_, sizeof(OrderReplace));
                replace.length = swap_uint16(replace.length);
                replace.stock_locate = swap_uint16(replace.stock_locate);
                replace.tracking_number = swap_uint16(replace.tracking_number);
                replace.timestamp = swap_uint64(replace.timestamp);
                replace.original_order_reference = swap_uint64(replace.original_order_reference);
                replace.new_order_reference = swap_uint64(replace.new_order_reference);
                replace.shares = swap_uint32(replace.shares);
                replace.price = swap_uint32(replace.price);
                result = replace;
            }
            break;
        }
        case 'P': {
            if (msg_length >= sizeof(Trade)) {
                Trade trade;
                std::memcpy(&trade, buffer_ + offset_, sizeof(Trade));
                trade.length = swap_uint16(trade.length);
                trade.stock_locate = swap_uint16(trade.stock_locate);
                trade.tracking_number = swap_uint16(trade.tracking_number);
                trade.timestamp = swap_uint64(trade.timestamp);
                trade.order_reference = swap_uint64(trade.order_reference);
                trade.shares = swap_uint32(trade.shares);
                trade.price = swap_uint32(trade.price);
                trade.match_number = swap_uint64(trade.match_number);
                result = trade;
            }
            break;
        }
        default:
            break;
    }

    offset_ += msg_length;
    return result;
}

}
