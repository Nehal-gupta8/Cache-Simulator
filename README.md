# Cache Simulator Project

A C++ implementation of different cache replacement policies (LRU, LFU) with benchmarking and file persistence capabilities.

## Features

- **Multiple Cache Policies**:
  - LRU (Least Recently Used)
  - LFU (Least Frequently Used)
  
- **Interactive Menu System**:
  - Add keys to cache
  - View cache contents
  - Display statistics (hit/miss ratios)
  - Save/load cache state
  - Run performance benchmarks
  - Clear cache

- **Advanced Functionality**:
  - File persistence (save/load cache state)
  - Performance benchmarking
  - Access tracking and statistics

## How to Compile and Run

1. **Compile the program**:
   ```bash
   g++ cache_simulator.cpp -o cache_sim -std=c++17
