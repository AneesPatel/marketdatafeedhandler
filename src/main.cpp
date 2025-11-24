#include "iex_parser.hpp"
#include "enhanced_order_book.hpp"
#include "itch_parser.hpp"
#include "order_book.hpp"
#include "websocket_server.hpp"
#include "tick_recorder.hpp"
#include "replay_engine.hpp"
#include "strategy.hpp"
#include "lock_free_queue.hpp"
#include "latency_tracker.hpp"
#include "pcap_reader.hpp"
#include "memory_pool.hpp"
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>
#include <sstream>
#include <random>
#include <iomanip>

std::atomic<bool> g_running{true};

void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        g_running = false;
    }
}

void demo_complete_system() {
    std::cout << "\n=== High-Performance Market Data Feed Handler Demo ===\n\n";
    
    std::cout << "Initializing components...\n";
    
    EnhancedOrderBook aapl_book("AAPL");
    EnhancedOrderBook msft_book("MSFT");
    EnhancedOrderBook googl_book("GOOGL");
    
    SPSCQueue<iex::QuoteUpdate> quote_queue(4096);
    MemoryPool<iex::QuoteUpdate> message_pool;
    
    TickRecorder recorder("demo_ticks.dat");
    
    TradingStrategy strategy;
    strategy.set_spread_threshold(0.05);
    strategy.set_imbalance_threshold(0.3);
    
    int signal_count = 0;
    strategy.set_signal_callback([&signal_count](const std::string& symbol, double price, 
                                                   uint64_t size, bool is_buy) {
        signal_count++;
        std::cout << "[SIGNAL #" << signal_count << "] " << symbol << " " 
                  << (is_buy ? "BUY" : "SELL") << " " 
                  << size << " @ $" << std::fixed << std::setprecision(2) 
                  << price << "\n";
    });
    
    std::cout << "\nSimulating live market data feed...\n";
    std::cout << "Processing 50,000 messages across 3 symbols\n\n";
    
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<int64_t> aapl_price(1480000, 1520000);
    std::uniform_int_distribution<int64_t> msft_price(3800000, 3850000);
    std::uniform_int_distribution<int64_t> googl_price(14000000, 14200000);
    std::uniform_int_distribution<uint64_t> size_dist(100, 5000);
    std::uniform_int_distribution<int> spread_dist(5, 50);
    
    perf::LatencyHistogram parse_latency;
    perf::LatencyHistogram book_latency;
    perf::LatencyHistogram e2e_latency;
    
    const size_t total_messages = 50000;
    uint64_t timestamp = 1700000000000000000ULL;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < total_messages && g_running; ++i) {
        auto e2e_start = perf::rdtsc_start();
        
        iex::QuoteUpdate quote{};
        quote.header.type = static_cast<uint8_t>(iex::MessageType::QuoteUpdate);
        quote.header.timestamp = timestamp;
        quote.flags = 0;
        
        EnhancedOrderBook* book = nullptr;
        int symbol_choice = i % 3;
        
        if (symbol_choice == 0) {
            std::memcpy(quote.symbol, "AAPL    ", 8);
            quote.bid_price = aapl_price(gen);
            book = &aapl_book;
        } else if (symbol_choice == 1) {
            std::memcpy(quote.symbol, "MSFT    ", 8);
            quote.bid_price = msft_price(gen);
            book = &msft_book;
        } else {
            std::memcpy(quote.symbol, "GOOGL   ", 8);
            quote.bid_price = googl_price(gen);
            book = &googl_book;
        }
        
        quote.bid_size = size_dist(gen);
        quote.ask_price = quote.bid_price + spread_dist(gen);
        quote.ask_size = size_dist(gen);
        
        auto parse_start = perf::rdtsc_start();
        quote_queue.try_push(quote);
        auto msg = quote_queue.try_pop();
        auto parse_end = perf::rdtsc_end();
        parse_latency.record(parse_end - parse_start);
        
        if (msg) {
            auto book_start = perf::rdtsc_start();
            
            std::string symbol = iex::symbol_to_string(msg->symbol);
            
            if (i % 5 == 0) {
                book->add_order(i * 2, 'B', msg->bid_price, msg->bid_size, timestamp);
                book->add_order(i * 2 + 1, 'S', msg->ask_price, msg->ask_size, timestamp);
            } else if (i % 7 == 0 && i > 0) {
                book->execute_order((i - 1) * 2, msg->bid_size / 4, timestamp);
            } else if (i % 11 == 0 && i > 0) {
                book->cancel_order((i - 1) * 2 + 1, msg->ask_size / 3, timestamp);
            } else {
                book->modify_order(i * 2, msg->bid_size + 100, timestamp);
            }
            
            auto book_end = perf::rdtsc_end();
            book_latency.record(book_end - book_start);
            
            recorder.record_quote(timestamp, symbol, 
                                msg->bid_price, msg->bid_size,
                                msg->ask_price, msg->ask_size);
            
            if (i % 1000 == 0) {
                auto snapshot = book->snapshot();
                strategy.on_quote_update(snapshot);
            }
        }
        
        auto e2e_end = perf::rdtsc_end();
        e2e_latency.record(e2e_end - e2e_start);
        
        timestamp += 100000 + (gen() % 500000);
        
        if ((i + 1) % 10000 == 0) {
            std::cout << "  Processed: " << (i + 1) << " / " << total_messages 
                      << " (" << std::fixed << std::setprecision(1)
                      << (100.0 * (i + 1) / total_messages) << "%)\n";
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time).count();
    
    recorder.flush();
    
    std::cout << "\n=== Performance Results ===\n\n";
    
    std::cout << "Messages Processed: " << total_messages << "\n";
    std::cout << "Total Time: " << elapsed_ms << " ms\n";
    std::cout << "Throughput: " << std::fixed << std::setprecision(0) 
              << (total_messages * 1000.0 / elapsed_ms) << " msgs/sec\n\n";
    
    double cpu_freq = 3.0;
    
    std::cout << "Message Queue Latency:\n";
    std::cout << "  Average: " << std::fixed << std::setprecision(1) 
              << perf::cycles_to_ns(parse_latency.average(), cpu_freq) << " ns\n";
    std::cout << "  p50:     " << perf::cycles_to_ns(parse_latency.percentile(0.50), cpu_freq) << " ns\n";
    std::cout << "  p95:     " << perf::cycles_to_ns(parse_latency.percentile(0.95), cpu_freq) << " ns\n";
    std::cout << "  p99:     " << perf::cycles_to_ns(parse_latency.percentile(0.99), cpu_freq) << " ns\n";
    std::cout << "  p99.9:   " << perf::cycles_to_ns(parse_latency.percentile(0.999), cpu_freq) << " ns\n\n";
    
    std::cout << "Order Book Update Latency:\n";
    std::cout << "  Average: " << perf::cycles_to_ns(book_latency.average(), cpu_freq) << " ns\n";
    std::cout << "  p50:     " << perf::cycles_to_ns(book_latency.percentile(0.50), cpu_freq) << " ns\n";
    std::cout << "  p95:     " << perf::cycles_to_ns(book_latency.percentile(0.95), cpu_freq) << " ns\n";
    std::cout << "  p99:     " << perf::cycles_to_ns(book_latency.percentile(0.99), cpu_freq) << " ns\n";
    std::cout << "  p99.9:   " << perf::cycles_to_ns(book_latency.percentile(0.999), cpu_freq) << " ns\n\n";
    
    std::cout << "End-to-End Pipeline Latency:\n";
    std::cout << "  Average: " << std::setprecision(2) 
              << perf::cycles_to_us(e2e_latency.average(), cpu_freq) << " μs\n";
    std::cout << "  p50:     " << perf::cycles_to_us(e2e_latency.percentile(0.50), cpu_freq) << " μs\n";
    std::cout << "  p95:     " << perf::cycles_to_us(e2e_latency.percentile(0.95), cpu_freq) << " μs\n";
    std::cout << "  p99:     " << perf::cycles_to_us(e2e_latency.percentile(0.99), cpu_freq) << " μs\n";
    std::cout << "  p99.9:   " << perf::cycles_to_us(e2e_latency.percentile(0.999), cpu_freq) << " μs\n\n";
    
    std::cout << "Order Book State:\n";
    auto aapl_snap = aapl_book.snapshot();
    auto msft_snap = msft_book.snapshot();
    auto googl_snap = googl_book.snapshot();
    
    std::cout << "  AAPL:  Bid $" << std::fixed << std::setprecision(2) 
              << (aapl_snap.best_bid / 10000.0) << " (" << aapl_snap.best_bid_size 
              << ") | Ask $" << (aapl_snap.best_ask / 10000.0) 
              << " (" << aapl_snap.best_ask_size << ") | Spread: $" 
              << aapl_snap.spread << " | Orders: " << aapl_book.total_orders() << "\n";
    
    std::cout << "  MSFT:  Bid $" << (msft_snap.best_bid / 10000.0) 
              << " (" << msft_snap.best_bid_size 
              << ") | Ask $" << (msft_snap.best_ask / 10000.0) 
              << " (" << msft_snap.best_ask_size << ") | Spread: $" 
              << msft_snap.spread << " | Orders: " << msft_book.total_orders() << "\n";
    
    std::cout << "  GOOGL: Bid $" << (googl_snap.best_bid / 10000.0) 
              << " (" << googl_snap.best_bid_size 
              << ") | Ask $" << (googl_snap.best_ask / 10000.0) 
              << " (" << googl_snap.best_ask_size << ") | Spread: $" 
              << googl_snap.spread << " | Orders: " << googl_book.total_orders() << "\n\n";
    
    std::cout << "Trading Signals Generated: " << signal_count << "\n";
    std::cout << "Ticks Recorded: " << recorder.count() << "\n\n";
    
    std::cout << "=== Demo Complete ===\n";
    std::cout << "Data saved to: demo_ticks.dat\n";
}

void demo_itch_processing() {
    std::cout << "\n=== NASDAQ ITCH 5.0 Processing Demo ===\n\n";
    
    EnhancedOrderBook book("AAPL");
    
    std::cout << "Simulating ITCH message stream...\n\n";
    
    std::vector<uint8_t> itch_messages;
    
    uint64_t order_id_counter = 1000000;
    uint64_t timestamp = 34200000000000ULL;
    
    for (int i = 0; i < 100; ++i) {
        if (i % 3 == 0) {
            itch::AddOrder add{};
            add.length = itch::swap_uint16(sizeof(itch::AddOrder));
            add.type = 'A';
            add.stock_locate = itch::swap_uint16(1);
            add.tracking_number = itch::swap_uint16(i);
            add.timestamp = itch::swap_uint64(timestamp);
            add.order_reference = itch::swap_uint64(order_id_counter++);
            add.buy_sell = (i % 2 == 0) ? 'B' : 'S';
            add.shares = itch::swap_uint32(100 + i * 10);
            std::memcpy(add.stock, "AAPL    ", 8);
            add.price = itch::swap_uint32(1500000 + i * 100);
            
            const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&add);
            itch_messages.insert(itch_messages.end(), ptr, ptr + sizeof(add));
        }
        
        timestamp += 1000000;
    }
    
    itch::Parser parser(itch_messages.data(), itch_messages.size());
    
    perf::LatencyHistogram parse_hist;
    size_t msg_count = 0;
    
    while (parser.has_more()) {
        auto start = perf::rdtsc_start();
        auto msg = parser.parse_next();
        auto end = perf::rdtsc_end();
        
        parse_hist.record(end - start);
        
        if (msg) {
            msg_count++;
            
            if (auto* add = std::get_if<itch::AddOrder>(&*msg)) {
                std::string stock = itch::stock_to_string(add->stock);
                char side = add->buy_sell;
                int64_t price = add->price;
                uint64_t shares = add->shares;
                uint64_t order_ref = add->order_reference;
                
                book.add_order(order_ref, side, price, shares, add->timestamp);
                
                if (msg_count <= 10) {
                    std::cout << "  [ADD] Order " << order_ref << " | " << stock << " | "
                              << side << " | " << shares << " @ $" << std::fixed 
                              << std::setprecision(2) << (price / 10000.0) << "\n";
                }
            }
        }
    }
    
    std::cout << "\nProcessed " << msg_count << " ITCH messages\n";
    std::cout << "Average parse latency: " << std::fixed << std::setprecision(1)
              << perf::cycles_to_ns(parse_hist.average(), 3.0) << " ns\n";
    
    auto snapshot = book.snapshot();
    std::cout << "\nFinal Order Book:\n";
    std::cout << "  Best Bid: $" << std::fixed << std::setprecision(2) 
              << (snapshot.best_bid / 10000.0) << " (" << snapshot.best_bid_size << ")\n";
    std::cout << "  Best Ask: $" << (snapshot.best_ask / 10000.0) 
              << " (" << snapshot.best_ask_size << ")\n";
    std::cout << "  Total Orders: " << book.total_orders() << "\n";
}

void generate_sample_data(const std::string& filename) {
    std::cout << "Generating sample market data...\n";
    
    TickRecorder recorder(filename);
    
    const char* symbols[] = {"AAPL", "MSFT", "GOOGL", "AMZN", "TSLA"};
    
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<int64_t> price_dist(1000000, 5000000);
    std::uniform_int_distribution<uint64_t> size_dist(100, 10000);
    
    uint64_t timestamp = 1000000000000000;
    
    for (size_t i = 0; i < 10000; ++i) {
        const char* symbol = symbols[i % 5];
        
        int64_t bid_price = price_dist(gen);
        int64_t ask_price = bid_price + 10 + (gen() % 100);
        uint64_t bid_size = size_dist(gen);
        uint64_t ask_size = size_dist(gen);
        
        recorder.record_quote(timestamp, symbol, 
                            bid_price, bid_size, 
                            ask_price, ask_size);
        
        if (i % 10 == 0) {
            int64_t trade_price = bid_price + ((ask_price - bid_price) / 2);
            uint64_t trade_size = size_dist(gen) / 10;
            recorder.record_trade(timestamp, symbol, 
                                trade_price, trade_size, 
                                gen() % 2);
        }
        
        timestamp += 1000000 + (gen() % 5000000);
    }
    
    recorder.flush();
    std::cout << "Generated " << recorder.count() << " records\n";
}

int main(int argc, char* argv[]) {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    
    std::cout << "╔══════════════════════════════════════════════════════════╗\n";
    std::cout << "║  High-Performance Market Data Feed Handler              ║\n";
    std::cout << "║  C++17 | Lock-Free | Sub-10μs Latency                   ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════╝\n";
    
    std::string mode = argc > 1 ? argv[1] : "demo";
    
    if (mode == "demo" || mode == "live") {
        demo_complete_system();
    } else if (mode == "itch") {
        demo_itch_processing();
    } else {
        std::cout << "\nUsage: " << argv[0] << " [mode]\n";
        std::cout << "\nModes:\n";
        std::cout << "  demo  - Run complete system demonstration (default)\n";
        std::cout << "  itch  - NASDAQ ITCH 5.0 processing demo\n";
        std::cout << "  live  - Same as demo\n\n";
    }
    
    
    return 0;
}
