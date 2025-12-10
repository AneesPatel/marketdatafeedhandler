# Market Data Feed Handler

High-performance C++17 market data feed handler with NASDAQ ITCH 5.0 protocol parser.

## Features

- NASDAQ ITCH 5.0 message parsing
- Lock-free data structures for concurrent access
- Memory pool allocator for zero-allocation hot path
- Order book management
- RDTSC-based latency measurement

## Performance

Measured on Intel Core processor at 3.99 GHz with 50,000 ITCH messages:

| Metric | Value |
|--------|-------|
| Throughput | 45M msgs/sec |
| Parse Latency (mean) | 9.8 ns |
| Parse Latency (p50) | 10 ns |
| Parse Latency (p99) | 20 ns |
| Parse Latency (p99.9) | 40 ns |

### Sample Output

```
$ ./benchmark.exe
Loaded 50000 messages (1800000 bytes)
Calibrating CPU frequency...
CPU frequency: 3992.47 MHz
Warmup...
Running benchmark...

=== BENCHMARK RESULTS ===

Messages parsed: 50000
Total time: 1.098 ms
Throughput: 45542054 msgs/sec

Latency (nanoseconds):
  Min:  0.3 ns
  Mean: 9.8 ns
  P50:  10.0 ns
  P90:  10.0 ns
  P99:  20.0 ns
  P99.9:40.1 ns
  Max:  250.3 ns
```

## Build

Requires C++17 compiler with optimization support.

```bash
g++ -std=c++17 -O3 -march=native -I include -o benchmark.exe benchmarks/full_pipeline_benchmark.cpp src/marketdata_parser.cpp
```

## Run Benchmark

First generate sample ITCH data:

```bash
python data/generate_sample_itch.py
```

Then run the benchmark:

```bash
./benchmark.exe
```

## Project Structure

```
include/
  marketdata_parser.hpp    ITCH parser interface
  timer.hpp                RDTSC timing utilities
  latency_histogram.hpp    Latency measurement
  lock_free_queue.hpp      SPSC queue
  memory_pool.hpp          Pool allocator
  order_book.hpp           Order book implementation

src/
  marketdata_parser.cpp    ITCH parser implementation

benchmarks/
  full_pipeline_benchmark.cpp    Main benchmark

data/
  generate_sample_itch.py        ITCH data generator
```

## ITCH 5.0 Message Format

The parser handles Add Order messages (type 'A'):

| Field | Offset | Size | Description |
|-------|--------|------|-------------|
| Message Type | 0 | 1 | 'A' for Add Order |
| Stock Locate | 1 | 2 | Stock identifier |
| Tracking Number | 3 | 2 | Sequence number |
| Timestamp | 5 | 6 | Nanoseconds since midnight |
| Order Reference | 11 | 8 | Unique order ID |
| Buy/Sell | 19 | 1 | 'B' or 'S' |
| Shares | 20 | 4 | Number of shares |
| Stock | 24 | 8 | Stock symbol |
| Price | 32 | 4 | Price (4 decimal places) |

All multi-byte fields are big-endian.

## License

MIT
