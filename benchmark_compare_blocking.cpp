#include <algorithm>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <random>
#include <thread>
#include <tuple>
#include <vector>

#include "matrix.hpp"

using Clock = std::chrono::high_resolution_clock;

struct IdealMatrix {
    size_t rows;
    size_t cols;
    std::vector<int> data;

    IdealMatrix(size_t r, size_t c) : rows(r), cols(c), data(r * c, 0) {}

    int& operator()(size_t r, size_t c) {
        return data[r * cols + c];
    }

    const int& operator()(size_t r, size_t c) const {
        return data[r * cols + c];
    }
};

static std::vector<int> make_random_flat(size_t rows, size_t cols, int seed) {
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> dist(1, 10);
    std::vector<int> out(rows * cols);
    for (size_t i = 0; i < out.size(); ++i) out[i] = dist(rng);
    return out;
}

static matrix<int> make_library_matrix(size_t rows, size_t cols, const std::vector<int>& flat) {
    matrix<int> m(rows, cols);
    for (size_t i = 0; i < rows; ++i) {
        for (size_t j = 0; j < cols; ++j) {
            m(i, j) = flat[i * cols + j];
        }
    }
    return m;
}

static IdealMatrix make_ideal_matrix(size_t rows, size_t cols, const std::vector<int>& flat) {
    IdealMatrix m(rows, cols);
    m.data = flat;
    return m;
}

static IdealMatrix multiply_naive_single_thread(const IdealMatrix& a, const IdealMatrix& b) {
    IdealMatrix c(a.rows, b.cols);

    for (size_t i = 0; i < a.rows; ++i) {
        for (size_t k = 0; k < a.cols; ++k) {
            const int aik = a(i, k);
            for (size_t j = 0; j < b.cols; ++j) {
                c(i, j) += aik * b(k, j);
            }
        }
    }

    return c;
}

static void naive_multiplication_loop(const IdealMatrix& a, const IdealMatrix& b, IdealMatrix& c, size_t start, size_t end) {
    for (size_t i = start; i < end; ++i) {
        for (size_t j = 0; j < c.cols; ++j) {
            c(i, j) = 0;
        }
        for (size_t k = 0; k < a.cols; ++k) {
            const int aik = a(i, k);
            for (size_t j = 0; j < c.cols; ++j) {
                c(i, j) += aik * b(k, j);
            }
        }
    }
}

static IdealMatrix multiply_naive_multi_thread(const IdealMatrix& a, const IdealMatrix& b, size_t threadCount) {
    IdealMatrix c(a.rows, b.cols);

    size_t hardThreads = std::max<size_t>(1, threadCount);
    size_t maxThreads = std::min(hardThreads, std::max<size_t>(1, a.rows));

    size_t rowsPerThread = c.rows / maxThreads;
    std::unique_ptr<std::thread[]> threadArr(new std::thread[maxThreads]);

    for (size_t i = 0; i < maxThreads; ++i) {
        if (i == maxThreads - 1) {
            threadArr[i] = std::thread(naive_multiplication_loop, std::cref(a), std::cref(b), std::ref(c), rowsPerThread * i, c.rows);
            break;
        }
        threadArr[i] = std::thread(naive_multiplication_loop, std::cref(a), std::cref(b), std::ref(c), rowsPerThread * i, rowsPerThread * (i + 1));
    }

    for (size_t i = 0; i < maxThreads; ++i) threadArr[i].join();

    return c;
}

static IdealMatrix multiply_blocked_ideal(const IdealMatrix& a, const IdealMatrix& b, size_t blockSize, size_t threadCount) {
    IdealMatrix c(a.rows, b.cols);

    const size_t m = a.rows;
    const size_t kDim = a.cols;
    const size_t n = b.cols;
    const int* aData = a.data.data();
    const int* bData = b.data.data();
    int* cData = c.data.data();

    const size_t hw = std::max<size_t>(1, threadCount);
    const size_t totalWork = m * n;
    const size_t workThresholdPerThread = 128 * 128;
    const size_t allowedByWork = std::max<size_t>(1, totalWork / workThresholdPerThread);
    const size_t threadsToUse = std::min<size_t>(std::min<size_t>(hw, std::max<size_t>(1, m)), allowedByWork);
    const size_t rowsPerThread = (m + threadsToUse - 1) / threadsToUse;

    std::vector<std::thread> workers;
    workers.reserve(threadsToUse);

    for (size_t t = 0; t < threadsToUse; ++t) {
        const size_t rowBegin = t * rowsPerThread;
        const size_t rowEnd = std::min(m, rowBegin + rowsPerThread);
        if (rowBegin >= rowEnd) continue;

        workers.emplace_back([&, rowBegin, rowEnd]() {
            for (size_t ii = rowBegin; ii < rowEnd; ii += blockSize) {
                const size_t iMax = std::min(rowEnd, ii + blockSize);
                for (size_t jj = 0; jj < n; jj += blockSize) {
                    const size_t jMax = std::min(n, jj + blockSize);
                    for (size_t kk = 0; kk < kDim; kk += blockSize) {
                        const size_t kMax = std::min(kDim, kk + blockSize);

                        for (size_t i = ii; i < iMax; ++i) {
                            int* cRow = cData + i * n;
                            const int* aRow = aData + i * kDim;
                            for (size_t k = kk; k < kMax; ++k) {
                                const int aik = aRow[k];
                                const int* bRow = bData + k * n;
                                size_t j = jj;
                                for (; j + 3 < jMax; j += 4) {
                                    cRow[j] += aik * bRow[j];
                                    cRow[j + 1] += aik * bRow[j + 1];
                                    cRow[j + 2] += aik * bRow[j + 2];
                                    cRow[j + 3] += aik * bRow[j + 3];
                                }
                                for (; j < jMax; ++j) {
                                    cRow[j] += aik * bRow[j];
                                }
                            }
                        }
                    }
                }
            }
        });
    }

    for (size_t i = 0; i < workers.size(); ++i) workers[i].join();
    return c;
}

template <typename F>
static double average_seconds(int runs, F&& fn) {
    double total = 0.0;
    for (int r = 0; r < runs; ++r) {
        const auto start = Clock::now();
        fn();
        const auto end = Clock::now();
        total += std::chrono::duration<double>(end - start).count();
    }
    return total / runs;
}

static bool equal_results(const matrix<int>& lib, const IdealMatrix& ideal) {
    if (lib.rows() != ideal.rows || lib.cols() != ideal.cols) return false;
    for (size_t i = 0; i < lib.rows(); ++i) {
        for (size_t j = 0; j < lib.cols(); ++j) {
            if (lib(i, j) != ideal(i, j)) return false;
        }
    }
    return true;
}

int main() {
    const size_t idealBlockSize = 64;
    const int runs = 2;
    const size_t hwThreads = std::max(1u, std::thread::hardware_concurrency());

    std::vector<std::tuple<size_t, size_t, size_t>> cases = {
        {256, 256, 256},
        {512, 512, 512},
        {800, 800, 800},
        {384, 256, 512},
        {5, 4, 10}
    };

    std::cout << "Hardware threads: " << hwThreads << "\n";
    std::cout << "Ideal block size: " << idealBlockSize << "\n";

    for (size_t idx = 0; idx < cases.size(); ++idx) {
        const size_t m = std::get<0>(cases[idx]);
        const size_t k = std::get<1>(cases[idx]);
        const size_t n = std::get<2>(cases[idx]);

        const auto aFlat = make_random_flat(m, k, 100 + static_cast<int>(idx));
        const auto bFlat = make_random_flat(k, n, 200 + static_cast<int>(idx));

        auto libA = make_library_matrix(m, k, aFlat);
        auto libB = make_library_matrix(k, n, bFlat);
        auto idealA = make_ideal_matrix(m, k, aFlat);
        auto idealB = make_ideal_matrix(k, n, bFlat);

        long long checksumLib = 0;
        long long checksumNaive = 0;
        long long checksumNaiveMT = 0;
        long long checksumIdeal = 0;

        const double naiveTime = average_seconds(runs, [&]() {
            auto c = multiply_naive_single_thread(idealA, idealB);
            checksumNaive += c(0, 0) + c(c.rows - 1, c.cols - 1);
        });

        const double naiveMultiTime = average_seconds(runs, [&]() {
            auto c = multiply_naive_multi_thread(idealA, idealB, hwThreads);
            checksumNaiveMT += c(0, 0) + c(c.rows - 1, c.cols - 1);
        });

        const double libraryTime = average_seconds(runs, [&]() {
            auto c = libA * libB;
            checksumLib += c(0, 0) + c(c.rows() - 1, c.cols() - 1);
        });

        const double idealTime = average_seconds(runs, [&]() {
            auto c = multiply_blocked_ideal(idealA, idealB, idealBlockSize, hwThreads);
            checksumIdeal += c(0, 0) + c(c.rows - 1, c.cols - 1);
        });

        auto libOut = libA * libB;
        auto idealOut = multiply_blocked_ideal(idealA, idealB, idealBlockSize, hwThreads);
        const bool same = equal_results(libOut, idealOut);

        std::cout << "\nCase A(" << m << "x" << k << ") * B(" << k << "x" << n << ")\n";
        std::cout << "Naive single-thread: " << naiveTime << " sec\n";
        std::cout << "Naive multi-thread : " << naiveMultiTime << " sec\n";
        std::cout << "Library time : " << libraryTime << " sec\n";
        std::cout << "Ideal time   : " << idealTime << " sec\n";
        std::cout << "Naive/Library: " << (naiveTime / libraryTime) << "x\n";
        std::cout << "NaiveMT/Library: " << (naiveMultiTime / libraryTime) << "x\n";
        std::cout << "Naive/Ideal  : " << (naiveTime / idealTime) << "x\n";
        std::cout << "NaiveMT/Ideal : " << (naiveMultiTime / idealTime) << "x\n";
        std::cout << "Library/Ideal: " << (libraryTime / idealTime) << "x\n";
        std::cout << "Result check : " << (same ? "MATCH" : "MISMATCH") << "\n";
        std::cout << "Checksums    : naive=" << checksumNaive << ", naiveMT=" << checksumNaiveMT << ", lib=" << checksumLib << ", ideal=" << checksumIdeal << "\n";
    }

    return 0;
}
