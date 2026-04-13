# Multithreaded Matrix Library

A high-performance C++ matrix library featuring multithreaded blocked matrix multiplication with configurable block sizes and benchmarking utilities.

## Overview

This project implements an optimized matrix class template with the following key features:

- **Efficient Matrix Operations**: Addition, multiplication, scalar operations, and transpose
- **Multithreaded Multiplication**: Automatic hardware-aware thread allocation for matrix multiplication
- **Block-Based Multiplication**: Configurable block size that can be tuned for cache optimization
- **Performance Benchmarking**: Comprehensive benchmarking tools to evaluate different blocking strategies
- **Modern C++**: Move semantics, copy/move constructors, and RAII principles

## Features

### Core Matrix Operations
- ✅ Matrix addition and in-place addition
- ✅ Matrix multiplication with multithreading support
- ✅ Scalar multiplication
- ✅ Matrix transpose
- ✅ 2D indexing via `operator()(row, col)`
- ✅ Move semantics for efficient memory management

### Performance Optimizations
- **Block-Based Multiplication**: Divides the computation into cache-friendly blocks
- **Automatic Thread Allocation**: Detects hardware concurrency and distributes work across available threads
- **Configurable Block Size**: Dynamically adjustable `blockSize` member for tuning performance
- **Register Reuse**: Inner loop optimization to maximize CPU cache efficiency
- **Thread Pooling**: Work-stealing approach distributes blocks across threads

### Benchmarking & Analysis
- Basic performance benchmark comparison
- Block size sweep analysis to find optimal configurations
- Single-threaded vs. multithreaded performance comparison
- Correctness validation against naive implementations

## Project Structure

```
.
├── matrix.hpp                      # Main matrix class template (253 lines)
├── benchmark.cpp                   # Basic performance benchmark
├── benchmark_compare_blocking.cpp  # Compare single vs. multithreaded performance
├── benchmark_blocksize_sweep.cpp   # Find optimal block size
└── README.md                       # This file
```

## File Descriptions

### `matrix.hpp`
The core library containing the `matrix<T>` template class.

**Key Methods:**
- `matrix(size_t rows, size_t cols)` - Constructor
- `operator*()` - Blocked multithreaded matrix multiplication
- `operator+()` - Matrix addition
- `operator*(T scalar)` - Scalar multiplication
- `transpose()` - Matrix transpose
- `rows()`, `cols()` - Dimension getters

**Configurable Parameter:**
```cpp
size_t blockSize = 64;  // Adjustable for performance tuning
```

### `benchmark.cpp`
Basic benchmark demonstrating:
- Single-threaded naive multiplication (`multiply_single`)
- Multithreaded blocked multiplication (`operator*`)
- Performance timing using `std::chrono::high_resolution_clock`
- Random matrix generation

### `benchmark_compare_blocking.cpp`
Compares the performance between:
- Naive single-threaded implementation
- Optimized multithreaded blocked implementation
- Validates correctness of results
- Reports speedup metrics

### `benchmark_blocksize_sweep.cpp`
Sweeps through different block sizes to find the optimal configuration:
- Tests various block sizes (e.g., 32, 64, 128, 256, etc.)
- Measures execution time for each configuration
- Identifies the sweet spot for cache performance on your hardware

## Building

### Requirements
- C++17 or later
- A modern C++ compiler (g++, clang, MSVC)
- Multi-core processor (for optimal multithreading performance)

### Compilation Examples

**Using g++ or clang:**
```bash
g++ -std=c++17 -O3 -pthread benchmark.cpp -o benchmark
g++ -std=c++17 -O3 -pthread benchmark_compare_blocking.cpp -o benchmark_compare_blocking
g++ -std=c++17 -O3 -pthread benchmark_blocksize_sweep.cpp -o benchmark_blocksize_sweep
```

**Optimization Notes:**
- Use `-O3` for maximum optimization
- Use `-pthread` or equivalent for threading support
- Consider `-march=native` for CPU-specific optimizations

## Usage

### Basic Usage

```cpp
#include "matrix.hpp"

int main() {
    // Create matrices
    matrix<int> A(n, m);
    matrix<int> B(m, p);
    
    // Perform multiplication (automatically uses multiple threads)
    matrix<int> C = A * B;
    
    // Access elements
    int value = C(i, j);
    
    // Adjust block size for performance tuning
    A.blockSize = 128;  // Default is 64
    
    return 0;
}
```

### Running Benchmarks

```bash
# Run basic benchmark
./benchmark

# Compare single vs. multithreaded performance
./benchmark_compare_blocking

# Find optimal block size for your hardware
./benchmark_blocksize_sweep
```

## Performance Characteristics

### Key Optimization Techniques

1. **Cache Blocking**: Divides matrices into blocks that fit in L1/L2 cache
2. **Register Reuse**: Keeps temporary values in registers to minimize memory access
3. **Hardware-Aware Threading**: Automatically scales with CPU core count
4. **Adaptive Thresholding**: Uses single thread for very small matrices (≤10,000 elements)
5. **Transpose Optimization**: Transposes the second matrix to improve cache locality

### When to Use

- **Best Performance**: Square or near-square matrices (100×100 to 10,000×10,000)
- **Good Parallelization**: When result matrix contains at least 10,000+ elements
- **Scaling**: Near-linear speedup up to available CPU cores

## Template Usage

The library is fully templated and supports any numeric type:

```cpp
matrix<float> fmat(100, 50);
matrix<double> dmat(50, 80);
matrix<int> imat(20, 30);
```

## Thread Model

- Automatic detection of hardware concurrency via `std::thread::hardware_concurrency()`
- Uses `std::unique_ptr<std::thread[]>` for dynamic thread allocation
- Distributes result blocks across threads
- Joins all threads before returning result

## Compiler Support

- ✅ GCC 7+
- ✅ Clang 5+
- ✅ MSVC 2017+

## Future Enhancements

- SIMD vectorization for inner loops
- GPU acceleration support
- Sparse matrix support
- LU decomposition and other factorizations
- Custom allocators for NUMA systems

## License

This project is provided as-is for educational and research purposes.

## Author

[Gaurang Gupta](https://github.com/Gaurang-2005)

## References

- Block matrix multiplication for cache optimization
- Modern C++ template metaprogramming
- Multithreaded performance optimization techniques

## Implementation Notes

- The core matrix implementation (`matrix.hpp`) has been fully hand-coded and optimized.
- Benchmarking and auxiliary files (`benchmark.cpp`, `benchmark_compare_blocking.cpp`, `benchmark_blocksize_sweep.cpp`) were generated with the assistance of AI tools and then adapted for this project.