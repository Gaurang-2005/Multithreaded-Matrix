#include <chrono>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <limits>
#include <random>
#include <tuple>
#include <vector>

#include "matrix.hpp"

using Clock = std::chrono::high_resolution_clock;

static std::vector<int> make_random_flat(size_t rows, size_t cols, int seed) {
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> dist(1, 10);
    std::vector<int> out(rows * cols);
    for (size_t i = 0; i < out.size(); ++i) out[i] = dist(rng);
    return out;
}

static matrix<int> make_matrix(size_t rows, size_t cols, const std::vector<int>& flat) {
    matrix<int> m(rows, cols);
    for (size_t i = 0; i < rows; ++i) {
        for (size_t j = 0; j < cols; ++j) {
            m(i, j) = flat[i * cols + j];
        }
    }
    return m;
}

static matrix<int> multiply_naive_single_thread(const matrix<int>& a, const matrix<int>& b) {
    matrix<int> c(a.rows(), b.cols());
    for (size_t i = 0; i < a.rows(); ++i) {
        for (size_t k = 0; k < a.cols(); ++k) {
            const int aik = a(i, k);
            for (size_t j = 0; j < b.cols(); ++j) {
                c(i, j) += aik * b(k, j);
            }
        }
    }
    return c;
}

static bool equal_matrix(const matrix<int>& x, const matrix<int>& y) {
    if (x.rows() != y.rows() || x.cols() != y.cols()) return false;
    for (size_t i = 0; i < x.rows(); ++i) {
        for (size_t j = 0; j < x.cols(); ++j) {
            if (x(i, j) != y(i, j)) return false;
        }
    }
    return true;
}

int main() {
    const int runs = 3;
    const std::vector<size_t> blockSizes = {8, 16, 24, 32, 48, 64, 96, 128};
    const std::vector<std::tuple<size_t, size_t, size_t>> cases = {
        {256, 256, 256},
        {512, 512, 512},
        {800, 800, 800},
        {384, 256, 512}
    };

    std::cout << "Block size sweep (library multiply only)\n";
    std::cout << "Runs per case: " << runs << "\n\n";

    std::vector<double> scoreByBlock(blockSizes.size(), 0.0);

    for (size_t c = 0; c < cases.size(); ++c) {
        const size_t m = std::get<0>(cases[c]);
        const size_t k = std::get<1>(cases[c]);
        const size_t n = std::get<2>(cases[c]);

        const auto aFlat = make_random_flat(m, k, 100 + static_cast<int>(c));
        const auto bFlat = make_random_flat(k, n, 200 + static_cast<int>(c));

        const auto refA = make_matrix(m, k, aFlat);
        const auto refB = make_matrix(k, n, bFlat);
        const auto ref = multiply_naive_single_thread(refA, refB);

        std::cout << "Case A(" << m << "x" << k << ") * B(" << k << "x" << n << ")\n";
        std::cout << std::left << std::setw(12) << "BlockSize" << std::setw(18) << "AvgTime(sec)" << "Status\n";

        double best = std::numeric_limits<double>::max();
        size_t bestBlock = blockSizes[0];

        for (size_t bi = 0; bi < blockSizes.size(); ++bi) {
            const size_t bs = blockSizes[bi];
            double total = 0.0;
            bool ok = true;

            for (int r = 0; r < runs; ++r) {
                auto a = make_matrix(m, k, aFlat);
                auto b = make_matrix(k, n, bFlat);
                a.blockSize = bs;

                const auto start = Clock::now();
                auto out = a * b;
                const auto end = Clock::now();
                total += std::chrono::duration<double>(end - start).count();

                if (r == 0 && !equal_matrix(out, ref)) {
                    ok = false;
                }
            }

            const double avg = total / runs;
            scoreByBlock[bi] += avg;
            if (avg < best) {
                best = avg;
                bestBlock = bs;
            }

            std::cout << std::left << std::setw(12) << bs
                      << std::setw(18) << std::fixed << std::setprecision(6) << avg
                      << (ok ? "MATCH" : "MISMATCH") << "\n";
        }

        std::cout << "Best for this case: blockSize=" << bestBlock
                  << " (" << std::fixed << std::setprecision(6) << best << " sec)\n\n";
    }

    double bestGlobal = std::numeric_limits<double>::max();
    size_t bestGlobalBlock = blockSizes[0];
    std::cout << "Overall score (sum of avg times across cases):\n";
    for (size_t bi = 0; bi < blockSizes.size(); ++bi) {
        std::cout << "  blockSize=" << std::setw(4) << blockSizes[bi]
                  << " total=" << std::fixed << std::setprecision(6) << scoreByBlock[bi] << " sec\n";
        if (scoreByBlock[bi] < bestGlobal) {
            bestGlobal = scoreByBlock[bi];
            bestGlobalBlock = blockSizes[bi];
        }
    }

    std::cout << "\nRecommended blockSize (global): " << bestGlobalBlock
              << " (lowest total time)\n";

    return 0;
}
