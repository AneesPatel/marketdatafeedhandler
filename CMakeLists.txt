cmake_minimum_required(VERSION 3.15)
project(MarketDataFeedHandler CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /O2 /GL /arch:AVX2)
    add_link_options(/LTCG)
else()
    add_compile_options(-Wall -Wextra -O3 -march=native -flto)
    add_link_options(-flto)
endif()

find_package(Threads REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(market_feed_handler
    src/main.cpp
    src/iex_parser.cpp
    src/order_book.cpp
    src/websocket_server.cpp
    src/tick_recorder.cpp
    src/replay_engine.cpp
    src/strategy.cpp
)

target_link_libraries(market_feed_handler PRIVATE Threads::Threads)

if(WIN32)
    target_link_libraries(market_feed_handler PRIVATE ws2_32)
endif()

add_executable(benchmark
    src/benchmark.cpp
    src/iex_parser.cpp
    src/order_book.cpp
)

target_link_libraries(benchmark PRIVATE Threads::Threads)
