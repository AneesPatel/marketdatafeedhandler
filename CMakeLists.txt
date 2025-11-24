cmake_minimum_required(VERSION 3.15)
project(MarketDataFeedHandler CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /O2 /GL /arch:AVX2 /fp:fast)
    add_link_options(/LTCG)
else()
    add_compile_options(-Wall -Wextra -O3 -march=native -flto -ffast-math)
    add_link_options(-flto)
endif()

option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(ENABLE_TSAN "Enable ThreadSanitizer" OFF)
option(ENABLE_PROFILING "Enable profiling symbols" OFF)

if(ENABLE_ASAN AND NOT MSVC)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
endif()

if(ENABLE_TSAN AND NOT MSVC)
    add_compile_options(-fsanitize=thread)
    add_link_options(-fsanitize=thread)
endif()

if(ENABLE_PROFILING AND NOT MSVC)
    add_compile_options(-g -fno-omit-frame-pointer)
endif()

find_package(Threads REQUIRED)

include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.12.1
)

FetchContent_Declare(
    googlebenchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG v1.8.3
)

set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest googlebenchmark)

include_directories(${CMAKE_SOURCE_DIR}/include)

add_library(feedhandler_core STATIC
    src/iex_parser.cpp
    src/itch_parser.cpp
    src/order_book.cpp
    src/enhanced_order_book.cpp
    src/websocket_server.cpp
    src/tick_recorder.cpp
    src/replay_engine.cpp
    src/strategy.cpp
    src/pcap_reader.cpp
)

target_link_libraries(feedhandler_core PRIVATE Threads::Threads)

if(WIN32)
    target_link_libraries(feedhandler_core PRIVATE ws2_32)
endif()

add_executable(market_feed_handler
    src/main.cpp
)

target_link_libraries(market_feed_handler PRIVATE feedhandler_core Threads::Threads)

if(WIN32)
    target_link_libraries(market_feed_handler PRIVATE ws2_32)
endif()

add_executable(benchmark
    src/benchmark.cpp
)

target_link_libraries(benchmark PRIVATE feedhandler_core Threads::Threads)

add_executable(advanced_benchmark
    src/advanced_benchmark.cpp
)

target_link_libraries(advanced_benchmark PRIVATE 
    feedhandler_core 
    benchmark::benchmark 
    Threads::Threads
)

enable_testing()

add_executable(unit_tests
    tests/test_iex_parser.cpp
    tests/test_itch_parser.cpp
    tests/test_order_book.cpp
    tests/test_enhanced_order_book.cpp
    tests/test_lock_free_queue.cpp
    tests/test_memory_pool.cpp
)

target_link_libraries(unit_tests PRIVATE 
    feedhandler_core 
    GTest::gtest_main 
    Threads::Threads
)

include(GoogleTest)
gtest_discover_tests(unit_tests)
