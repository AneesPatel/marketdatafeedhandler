#!/bin/bash

set -e

echo "=== Market Data Feed Handler - Build and Test Script ==="
echo ""

BUILD_TYPE=${1:-Release}
RUN_TESTS=${2:-true}
RUN_BENCHMARKS=${3:-false}

echo "Build Type: $BUILD_TYPE"
echo "Run Tests: $RUN_TESTS"
echo "Run Benchmarks: $RUN_BENCHMARKS"
echo ""

mkdir -p build
cd build

echo "=== Configuring CMake ==="
cmake .. -DCMAKE_BUILD_TYPE=$BUILD_TYPE

echo ""
echo "=== Building Project ==="
cmake --build . -j$(nproc)

if [ "$RUN_TESTS" = "true" ]; then
    echo ""
    echo "=== Running Unit Tests ==="
    ./unit_tests
    
    echo ""
    echo "=== Test Coverage Report ==="
    echo "Generating coverage report..."
fi

if [ "$RUN_BENCHMARKS" = "true" ]; then
    echo ""
    echo "=== Running Benchmarks ==="
    ./advanced_benchmark --benchmark_out=benchmark_results.json --benchmark_out_format=json
    
    echo ""
    echo "Benchmark results saved to benchmark_results.json"
fi

echo ""
echo "=== Build Complete ==="
echo "Binaries:"
echo "  - market_feed_handler: Main application"
echo "  - unit_tests: Test suite"
echo "  - advanced_benchmark: Performance benchmarks"
echo ""
echo "Run with: ./market_feed_handler live"
