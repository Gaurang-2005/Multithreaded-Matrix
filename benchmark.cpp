#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <random>
#include <vector>

#include "matrix.hpp"

using Clock = std::chrono::high_resolution_clock;

// ✅ standalone single-thread multiply
template<typename T>
void fill_random(matrix<T>& m) {
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(1, 10);

    for (size_t i = 0; i < m.rows(); i++) {
        for (size_t j = 0; j < m.cols(); j++) {
            m(i, j) = dist(rng);
        }
    }
}

static int parse_positive_int(const char* text, const char* name) {
    char* end = nullptr;
    const long value = std::strtol(text, &end, 10);
    if (end == text || *end != '\0' || value <= 0) {
        std::cerr << "Invalid " << name << ": " << text << "\n";
        std::exit(1);
    }
    return static_cast<int>(value);
}

int main() {
    // Usage: benchmark.exe <size> [runs] [warmup]
    if (__argc < 2 || __argc > 4) {
        std::cout << "Usage: " << __argv[0] << " <size> [runs=5] [warmup=1]\n";
        std::cout << "Example: " << __argv[0] << " 1024 7 2\n";
        return 1;
    }

    const int n = parse_positive_int(__argv[1], "size");
    const int runs = (__argc >= 3) ? parse_positive_int(__argv[2], "runs") : 5;
    const int warmup = (__argc >= 4) ? parse_positive_int(__argv[3], "warmup") : 1;

    matrix<int> A(static_cast<size_t>(n), static_cast<size_t>(n));
    matrix<int> B(static_cast<size_t>(n), static_cast<size_t>(n));
    fill_random(A);
    fill_random(B);

    for (int i = 0; i < warmup; ++i) {
        volatile int sink = (A * B)(0, 0);
        (void)sink;
    }

    std::vector<double> samples;
    samples.reserve(static_cast<size_t>(runs));
    long long checksum = 0;

    for (int i = 0; i < runs; ++i) {
        const auto start = Clock::now();
        matrix<int> C = A * B;
        const auto end = Clock::now();

        checksum += static_cast<long long>(C(0, 0));
        checksum += static_cast<long long>(C(C.rows() - 1, C.cols() - 1));
        samples.push_back(std::chrono::duration<double>(end - start).count());
    }

    std::sort(samples.begin(), samples.end());
    const double minTime = samples.front();
    const double maxTime = samples.back();
    const double medianTime = (samples.size() % 2 == 1)
        ? samples[samples.size() / 2]
        : (samples[samples.size() / 2 - 1] + samples[samples.size() / 2]) * 0.5;

    double sum = 0.0;
    for (double t : samples) sum += t;
    const double avgTime = sum / static_cast<double>(samples.size());

    std::cout << "Matrix size : " << n << " x " << n << "\n";
    std::cout << "Warmup runs : " << warmup << "\n";
    std::cout << "Measured runs: " << runs << "\n";
    std::cout << "Time (min)  : " << minTime << " sec\n";
    std::cout << "Time (median): " << medianTime << " sec\n";
    std::cout << "Time (avg)  : " << avgTime << " sec\n";
    std::cout << "Time (max)  : " << maxTime << " sec\n";
    std::cout << "Checksum    : " << checksum << "\n";

    return 0;
}