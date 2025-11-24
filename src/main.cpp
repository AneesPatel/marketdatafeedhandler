#include "iex_parser.hpp"
#include "order_book.hpp"
#include "websocket_server.hpp"
#include "tick_recorder.hpp"
#include "replay_engine.hpp"
#include "strategy.hpp"
#include "lock_free_queue.hpp"
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>
#include <sstream>

std::atomic<bool> g_running{true};

void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        g_running = false;
    }
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

void process_live_data() {
    std::cout << "\n=== Live Market Data Processing ===\n";
    
    OrderBookManager book_manager;
    WebSocketServer ws_server(8080);
    TickRecorder recorder("live_ticks.dat");
    TradingStrategy strategy;
    
    strategy.set_spread_threshold(0.02);
    strategy.set_imbalance_threshold(0.4);
    
    strategy.set_signal_callback([](const std::string& symbol, double price, 
                                    uint64_t size, bool is_buy) {
        std::cout << "[SIGNAL] " << symbol << " " 
                  << (is_buy ? "BUY" : "SELL") << " " 
                  << size << " @ $" << std::fixed << std::setprecision(2) 
                  << price << "\n";
    });
    
    ws_server.start();
    std::cout << "WebSocket server started on port 8080\n";
    
    std::vector<uint8_t> sample_buffer;
    
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<int64_t> price_dist(1500000, 1510000);
    std::uniform_int_distribution<uint64_t> size_dist(100, 1000);
    
    for (size_t i = 0; i < 1000; ++i) {
        iex::QuoteUpdate quote{};
        quote.header.type = static_cast<uint8_t>(iex::MessageType::QuoteUpdate);
        quote.header.timestamp = i * 1000000;
        quote.flags = 0;
        std::memcpy(quote.symbol, "AAPL    ", 8);
        quote.bid_price = price_dist(gen);
        quote.bid_size = size_dist(gen);
        quote.ask_price = quote.bid_price + 10 + (gen() % 20);
        quote.ask_size = size_dist(gen);
        
        const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&quote);
        sample_buffer.insert(sample_buffer.end(), ptr, ptr + sizeof(quote));
    }
    
    iex::Parser parser(sample_buffer.data(), sample_buffer.size());
    
    size_t msg_count = 0;
    auto start_time = std::chrono::high_resolution_clock::now();
    
    while (parser.has_more() && g_running) {
        auto msg_opt = parser.parse_next();
        if (!msg_opt) continue;
        
        auto& msg = *msg_opt;
        
        if (auto* quote = std::get_if<iex::QuoteUpdate>(&msg)) {
            std::string symbol = iex::symbol_to_string(quote->symbol);
            auto& book = book_manager.get_or_create(symbol);
            
            book.modify_bid(quote->bid_price, quote->bid_size, 
                          quote->header.timestamp);
            book.modify_ask(quote->ask_price, quote->ask_size, 
                          quote->header.timestamp);
            
            recorder.record_quote(quote->header.timestamp, symbol,
                                quote->bid_price, quote->bid_size,
                                quote->ask_price, quote->ask_size);
            
            auto snapshot = book.snapshot();
            
            if (msg_count % 100 == 0) {
                std::ostringstream json;
                json << "{\"symbol\":\"" << snapshot.symbol << "\","
                     << "\"bid\":" << (snapshot.best_bid / 10000.0) << ","
                     << "\"ask\":" << (snapshot.best_ask / 10000.0) << ","
                     << "\"spread\":" << snapshot.spread << ","
                     << "\"imbalance\":" << std::fixed << std::setprecision(3) 
                     << snapshot.imbalance << "}";
                
                ws_server.broadcast(json.str());
            }
            
            if (msg_count % 50 == 0) {
                strategy.on_quote_update(snapshot);
            }
            
            msg_count++;
            
            if (msg_count % 1000 == 0) {
                auto now = std::chrono::high_resolution_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - start_time).count();
                
                double msgs_per_sec = (msg_count * 1000.0) / elapsed;
                
                std::cout << "Processed: " << msg_count 
                          << " messages (" << std::fixed << std::setprecision(0)
                          << msgs_per_sec << " msgs/sec) - "
                          << "Clients: " << ws_server.client_count() << "\n";
            }
        }
    }
    
    recorder.flush();
    ws_server.stop();
    
    std::cout << "\nProcessed " << msg_count << " messages\n";
    std::cout << "Recorded " << recorder.count() << " ticks\n";
}

void replay_data() {
    std::cout << "\n=== Market Data Replay ===\n";
    
    const std::string filename = "sample_data.dat";
    
    if (std::ifstream(filename).good() == false) {
        generate_sample_data(filename);
    }
    
    OrderBookManager book_manager;
    WebSocketServer ws_server(8081);
    ws_server.start();
    
    std::cout << "WebSocket server started on port 8081\n";
    std::cout << "Starting replay at 10x speed...\n";
    
    ReplayEngine engine(filename);
    
    size_t msg_count = 0;
    auto start_time = std::chrono::high_resolution_clock::now();
    
    engine.set_callback([&](const TickRecord& record) {
        std::string symbol(record.symbol, 
            strnlen(record.symbol, sizeof(record.symbol)));
        
        auto& book = book_manager.get_or_create(symbol);
        
        if (record.flags & 0x01) {
            if (record.side == 0) {
                book.modify_bid(record.price, record.size, record.timestamp);
            } else {
                book.modify_ask(record.price, record.size, record.timestamp);
            }
        } else if (record.flags & 0x02) {
            if (record.side == 0) {
                book.modify_bid(record.price, record.size, record.timestamp);
            } else {
                book.modify_ask(record.price, record.size, record.timestamp);
            }
        }
        
        msg_count++;
        
        if (msg_count % 500 == 0) {
            auto snapshot = book.snapshot();
            
            std::ostringstream json;
            json << "{\"symbol\":\"" << snapshot.symbol << "\","
                 << "\"bid\":" << std::fixed << std::setprecision(2) 
                 << (snapshot.best_bid / 10000.0) << ","
                 << "\"ask\":" << (snapshot.best_ask / 10000.0) << ","
                 << "\"spread\":" << snapshot.spread << "}";
            
            ws_server.broadcast(json.str());
            
            std::cout << "\r" << symbol << " | Bid: $" 
                      << std::fixed << std::setprecision(2)
                      << (snapshot.best_bid / 10000.0) 
                      << " (" << snapshot.best_bid_size << ") | Ask: $"
                      << (snapshot.best_ask / 10000.0)
                      << " (" << snapshot.best_ask_size << ")" 
                      << std::flush;
        }
    });
    
    engine.set_speed(ReplayEngine::Speed::Fast10x);
    engine.start();
    
    while (engine.is_running() && g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time).count();
    
    std::cout << "\n\nReplay complete!\n";
    std::cout << "Messages replayed: " << msg_count << "\n";
    std::cout << "Time elapsed: " << elapsed << " ms\n";
    std::cout << "Average rate: " << std::fixed << std::setprecision(0)
              << (msg_count * 1000.0 / elapsed) << " msgs/sec\n";
    
    ws_server.stop();
}

int main(int argc, char* argv[]) {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    
    std::cout << "High-Performance Market Data Feed Handler\n";
    std::cout << "=========================================\n\n";
    
    std::string mode = "live";
    if (argc > 1) {
        mode = argv[1];
    }
    
    if (mode == "replay") {
        replay_data();
    } else if (mode == "generate") {
        generate_sample_data("sample_data.dat");
    } else {
        std::cout << "Usage: " << argv[0] << " [mode]\n";
        std::cout << "Modes:\n";
        std::cout << "  live     - Process live simulated data (default)\n";
        std::cout << "  replay   - Replay recorded data\n";
        std::cout << "  generate - Generate sample data file\n\n";
        
        process_live_data();
    }
    
    return 0;
}
