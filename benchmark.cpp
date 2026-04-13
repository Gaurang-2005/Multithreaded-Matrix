#include <cstddef>
#include <iostream>
#include <chrono>
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

template<typename T>
void print_matrix(const matrix<T>& m, const char* name) {
    std::cout << name << " (" << m.rows() << " x " << m.cols() << ")\n";
    for (size_t i = 0; i < m.rows(); i++) {
        for (size_t j = 0; j < m.cols(); j++) {
            std::cout << m(i, j) << ' ';
        }
        std::cout << '\n';
    }
}

int main() {
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> sizeDist(2, 5);

    const size_t rowsA = static_cast<size_t>(sizeDist(rng));
    const size_t common = static_cast<size_t>(sizeDist(rng));
    const size_t colsB = static_cast<size_t>(sizeDist(rng));

    matrix<int> A(rowsA, common);
    matrix<int> B(common, colsB);

    fill_random(A);
    fill_random(B);

    matrix<int> C = A * B;

    print_matrix(A, "A");
    print_matrix(B, "B");
    print_matrix(C, "A * B");

    return 0;
}