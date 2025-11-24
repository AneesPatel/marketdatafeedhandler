#!/bin/bash

set -e

echo "=== Generating Flame Graph ==="

if ! command -v perf &> /dev/null; then
    echo "Error: perf not found. Install with: sudo apt-get install linux-tools-common"
    exit 1
fi

if [ ! -f "flamegraph.pl" ]; then
    echo "Downloading FlameGraph scripts..."
    git clone https://github.com/brendangregg/FlameGraph.git
    ln -s FlameGraph/flamegraph.pl flamegraph.pl
    ln -s FlameGraph/stackcollapse-perf.pl stackcollapse-perf.pl
fi

echo "Building with profiling symbols..."
cd build
cmake .. -DENABLE_PROFILING=ON -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)
cd ..

echo ""
echo "Recording performance data..."
sudo perf record -F 99 -g --call-graph=dwarf -o perf.data ./build/market_feed_handler live &
PID=$!

echo "Running for 10 seconds..."
sleep 10

echo "Stopping profiler..."
sudo kill -SIGINT $PID
wait $PID 2>/dev/null || true

echo ""
echo "Generating flame graph..."
sudo perf script -i perf.data | ./stackcollapse-perf.pl | ./flamegraph.pl > flamegraph.svg

echo ""
echo "Flame graph generated: flamegraph.svg"
echo "Open with: firefox flamegraph.svg"
