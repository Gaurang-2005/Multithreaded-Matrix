#include <cstddef>
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

bool test(matrix<double> A, matrix<double> B) {
    matrix<double> res1 = A * B;

    matrix<double> res2(A.rows(), B.cols());

    std::cout<< "done"<<std::endl;
    for (size_t i = 0; i < A.rows(); i++) {
        for (size_t j = 0; j < B.cols(); j++) {
            res2(i, j) = 0;
        }
        for (size_t k = 0; k < A.cols(); k++) {
            double thisik = A(i, k);
            for (size_t j = 0; j < B.cols(); j++) {
                res2(i, j) += thisik * B(k, j);
            }
        }
    }  
    
    for(size_t i = 0; i < res1.cols(); i++) {
        for(size_t j = 0; j < res1.rows(); j++) {
            if (res1(j, i) != res2(j, i)) return false;
        }
    }
    return true;
}

int main() {
    std::cout << "Hardware Threads: " << std::thread::hardware_concurrency() << std::endl;
    std::vector<int> sizes = {100, 300, 500, 800, 1000, 10000};

    matrix<double> A(40, 40),  B(40,40);
    fill_random(A);
    fill_random(B);
    // std::vector<std::vector<double>> A1 = {{1, 2, 3},
    //                                         {4, 5, 6}};
    // std::vector<std::vector<double>> B1 = {{1, 2, 3},
    //                                         {4, 5, 6},
    //                                         {7, 8, 9}};
    // int a = 0;
    // for (auto& i : A1) {
    //     int b = 0;
    //     for (auto& j : i) {
    //         std::cout<<j<<" ";
    //         A(a, b++) = j;
    //     }
    //     std::cout<<std::endl;
    //     a++;
    // }
    // a = 0;
    // for (auto& i : B1) {
    //     int b = 0;
    //     for (auto& j : i) {
    //         std::cout<<j<<" ";
    //         B(a, b++) = j;
    //     }
    //     std::cout<<std::endl;
    //     a++;
    // }    
    std::cout<<"testing with matrix A and B: " << test(A, B) << std::endl;

    // for (int n : sizes) {
    //     std::cout << "\nMatrix size: " << n << " x " << n << "\n";

    //     matrix<double> A(n, n);
    //     matrix<double> B(n, n);

    //     fill_random(A);
    //     fill_random(B);

    //     double t1 = benchmark_single(A, B);
    //     double t2 = benchmark_multi(A, B);

    //     std::cout << "Single-thread: " << t1 << " sec\n";
    //     std::cout << "Multi-thread : " << t2 << " sec\n";

    //     if (t2 > 0)
    //        std::cout << "Speedup: " << t1 / t2 << "x\n";
    // }

    return 0;
}