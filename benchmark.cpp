#include <iostream>
#include <chrono>
#include <ostream>
#include <vector>
#include <random>
#include <cassert>

#include "matrix.hpp"

using Clock = std::chrono::high_resolution_clock;

// ✅ standalone single-thread multiply
template<typename T>
matrix<T> multiply_single(const matrix<T>& A, const matrix<T>& B) {
    assert(A.cols() == B.rows());

    matrix<T> result(A.rows(), B.cols());

    for (size_t i = 0; i < A.rows(); i++) {
        for (size_t j = 0; j < B.cols(); j++) {
            result(i, j) = 0;
        }
        for (size_t k = 0; k < A.cols(); k++) {
            T thisik = A(i, k);
            for (size_t j = 0; j < B.cols(); j++) {
                result(i, j) += thisik * B(k, j);
            }
        }
    }
    return result;
}

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

template<typename T>
double benchmark_single(matrix<T>& A, matrix<T>& B, int runs = 3) {
    double total = 0;

    for (int i = 0; i < runs; i++) {
        auto start = Clock::now();
        auto C = multiply_single(A, B);  // ✅ fixed
        auto end = Clock::now();

        total += std::chrono::duration<double>(end - start).count();
    }

    return total / runs;
}

template<typename T>
double benchmark_multi(matrix<T>& A, matrix<T>& B, int runs = 3) {
    double total = 0;

    for (int i = 0; i < runs; i++) {
        auto start = Clock::now();
        auto C = A * B;
        auto end = Clock::now();

        total += std::chrono::duration<double>(end - start).count();
    }

    return total / runs;
}

int main() {
    std::cout << "Hardware Threads: " << std::thread::hardware_concurrency() << std::endl;
    std::vector<int> sizes = {100, 300, 500, 800, 1000, 10000};

    for (int n : sizes) {
        std::cout << "\nMatrix size: " << n << " x " << n << "\n";

        matrix<double> A(n, n);
        matrix<double> B(n, n);

        fill_random(A);
        fill_random(B);

        double t1 = benchmark_single(A, B);
        double t2 = benchmark_multi(A, B);

        std::cout << "Single-thread: " << t1 << " sec\n";
        std::cout << "Multi-thread : " << t2 << " sec\n";

        if (t2 > 0)
           std::cout << "Speedup: " << t1 / t2 << "x\n";
    }

    return 0;
}