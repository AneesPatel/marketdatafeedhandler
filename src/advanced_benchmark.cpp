#include <benchmark/benchmark.h>
#include "iex_parser.hpp"
#include "itch_parser.hpp"
#include "order_book.hpp"
#include "enhanced_order_book.hpp"
#include "lock_free_queue.hpp"
#include "memory_pool.hpp"
#include "latency_tracker.hpp"
#include <random>

static void BM_IEXParsing(benchmark::State& state) {
    iex::QuoteUpdate quote{};
    quote.header.type = static_cast<uint8_t>(iex::MessageType::QuoteUpdate);
    quote.header.timestamp = 1000000;
    quote.flags = 0;
    std::memcpy(quote.symbol, "AAPL    ", 8);
    quote.bid_price = 1500000;
    quote.bid_size = 100;
    quote.ask_price = 1500100;
    quote.ask_size = 200;
    
    std::vector<uint8_t> buffer(sizeof(quote));
    std::memcpy(buffer.data(), &quote, sizeof(quote));
    
    for (auto _ : state) {
        iex::Parser parser(buffer.data(), buffer.size());
        auto msg = parser.parse_next();
        benchmark::DoNotOptimize(msg);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_IEXParsing);

static void BM_ITCHParsing(benchmark::State& state) {
    itch::AddOrder order{};
    order.length = itch::swap_uint16(sizeof(itch::AddOrder));
    order.type = 'A';
    order.stock_locate = itch::swap_uint16(0);
    order.tracking_number = itch::swap_uint16(1);
    order.timestamp = itch::swap_uint64(1000000);
    order.order_reference = itch::swap_uint64(12345);
    order.buy_sell = 'B';
    order.shares = itch::swap_uint32(100);
    std::memcpy(order.stock, "AAPL    ", 8);
    order.price = itch::swap_uint32(1500000);
    
    std::vector<uint8_t> buffer(sizeof(order));
    std::memcpy(buffer.data(), &order, sizeof(order));
    
    for (auto _ : state) {
        itch::Parser parser(buffer.data(), buffer.size());
        auto msg = parser.parse_next();
        benchmark::DoNotOptimize(msg);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ITCHParsing);

static void BM_OrderBookAddBid(benchmark::State& state) {
    OrderBook book("AAPL");
    uint64_t timestamp = 0;
    
    for (auto _ : state) {
        int64_t price = 1500000 + (timestamp % 100);
        book.add_bid(price, 100, timestamp++);
        benchmark::ClobberMemory();
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_OrderBookAddBid);

static void BM_OrderBookModifyBid(benchmark::State& state) {
    OrderBook book("AAPL");
    book.add_bid(1500000, 100, 0);
    uint64_t timestamp = 1;
    
    for (auto _ : state) {
        book.modify_bid(1500000, 100 + (timestamp % 50), timestamp++);
        benchmark::ClobberMemory();
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_OrderBookModifyBid);

static void BM_OrderBookBestBidAsk(benchmark::State& state) {
    OrderBook book("AAPL");
    book.add_bid(1500000, 100, 0);
    book.add_ask(1500100, 200, 1);
    
    for (auto _ : state) {
        auto bid = book.best_bid();
        auto ask = book.best_ask();
        benchmark::DoNotOptimize(bid);
        benchmark::DoNotOptimize(ask);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_OrderBookBestBidAsk);

static void BM_EnhancedOrderBookAddOrder(benchmark::State& state) {
    EnhancedOrderBook book("AAPL");
    uint64_t order_id = 0;
    
    for (auto _ : state) {
        int64_t price = 1500000 + (order_id % 100);
        book.add_order(order_id++, 'B', price, 100, order_id);
        benchmark::ClobberMemory();
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_EnhancedOrderBookAddOrder);

static void BM_EnhancedOrderBookExecuteOrder(benchmark::State& state) {
    EnhancedOrderBook book("AAPL");
    
    state.PauseTiming();
    for (uint64_t i = 0; i < 1000; ++i) {
        book.add_order(i, 'B', 1500000, 100, i);
    }
    state.ResumeTiming();
    
    uint64_t order_id = 0;
    for (auto _ : state) {
        book.execute_order(order_id++ % 1000, 10, order_id);
        benchmark::ClobberMemory();
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_EnhancedOrderBookExecuteOrder);

static void BM_SPSCQueuePushPop(benchmark::State& state) {
    SPSCQueue<uint64_t> queue(1024);
    uint64_t value = 0;
    
    for (auto _ : state) {
        queue.try_push(value++);
        auto result = queue.try_pop();
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations() * 2);
}
BENCHMARK(BM_SPSCQueuePushPop);

static void BM_MemoryPoolAllocation(benchmark::State& state) {
    MemoryPool<uint64_t> pool;
    std::vector<uint64_t*> ptrs;
    ptrs.reserve(100);
    
    for (auto _ : state) {
        auto* ptr = pool.allocate(42);
        benchmark::DoNotOptimize(ptr);
        ptrs.push_back(ptr);
        
        if (ptrs.size() >= 100) {
            for (auto* p : ptrs) {
                pool.deallocate(p);
            }
            ptrs.clear();
        }
    }
    
    for (auto* p : ptrs) {
        pool.deallocate(p);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_MemoryPoolAllocation);

static void BM_HeapAllocation(benchmark::State& state) {
    std::vector<uint64_t*> ptrs;
    ptrs.reserve(100);
    
    for (auto _ : state) {
        auto* ptr = new uint64_t(42);
        benchmark::DoNotOptimize(ptr);
        ptrs.push_back(ptr);
        
        if (ptrs.size() >= 100) {
            for (auto* p : ptrs) {
                delete p;
            }
            ptrs.clear();
        }
    }
    
    for (auto* p : ptrs) {
        delete p;
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_HeapAllocation);

static void BM_LatencyMeasurement(benchmark::State& state) {
    for (auto _ : state) {
        auto start = perf::rdtsc_start();
        benchmark::DoNotOptimize(start);
        auto end = perf::rdtsc_end();
        benchmark::DoNotOptimize(end);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LatencyMeasurement);

static void BM_EndToEndPipeline(benchmark::State& state) {
    SPSCQueue<iex::QuoteUpdate> queue(1024);
    OrderBook book("AAPL");
    MemoryPool<iex::QuoteUpdate> pool;
    
    iex::QuoteUpdate quote{};
    quote.header.type = static_cast<uint8_t>(iex::MessageType::QuoteUpdate);
    quote.header.timestamp = 1000000;
    quote.flags = 0;
    std::memcpy(quote.symbol, "AAPL    ", 8);
    quote.bid_price = 1500000;
    quote.bid_size = 100;
    quote.ask_price = 1500100;
    quote.ask_size = 200;
    
    for (auto _ : state) {
        queue.try_push(quote);
        
        auto msg = queue.try_pop();
        if (msg) {
            book.modify_bid(msg->bid_price, msg->bid_size, msg->header.timestamp);
            book.modify_ask(msg->ask_price, msg->ask_size, msg->header.timestamp);
            
            auto snapshot = book.snapshot();
            benchmark::DoNotOptimize(snapshot);
        }
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_EndToEndPipeline);

static void BM_OrderBookDepth(benchmark::State& state) {
    EnhancedOrderBook book("AAPL");
    
    state.PauseTiming();
    for (int i = 0; i < 100; ++i) {
        book.add_order(i, 'B', 1500000 - i * 100, 100, i);
        book.add_order(1000 + i, 'S', 1500100 + i * 100, 100, i);
    }
    state.ResumeTiming();
    
    for (auto _ : state) {
        auto bid_depth = book.get_bid_depth(10);
        auto ask_depth = book.get_ask_depth(10);
        benchmark::DoNotOptimize(bid_depth);
        benchmark::DoNotOptimize(ask_depth);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_OrderBookDepth);

BENCHMARK_MAIN();
