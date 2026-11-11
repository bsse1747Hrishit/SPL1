#pragma once
// ============================================================================
//  Random - small, fast, reproducible PRNG (xorshift64*).
//  Same seed => same draw, same fixtures, same season. Its state is stored
//  in save files so a loaded season continues exactly.
// ============================================================================
#include <cmath>
#include <utility>

namespace ds {

class Random {
public:
    explicit Random(unsigned long long seed = 0) { setSeed(seed); }

    void setSeed(unsigned long long seed) {
        state_ = seed ? seed : 0x2545F4914F6CDD1DULL;
        for (int i = 0; i < 8; ++i) next();   // warm up
    }
    unsigned long long state() const { return state_; }
    void setState(unsigned long long s) { state_ = s ? s : 0x2545F4914F6CDD1DULL; }

    unsigned long long next() {
        state_ ^= state_ >> 12;
        state_ ^= state_ << 25;
        state_ ^= state_ >> 27;
        return state_ * 2685821657736338717ULL;
    }
    // Uniform in [0, 1)
    double uniform() { return static_cast<double>(next() >> 11) * (1.0 / 9007199254740992.0); }
    // Uniform integer in [lo, hi]
    int range(int lo, int hi) {
        if (hi <= lo) return lo;
        return lo + static_cast<int>(next() % static_cast<unsigned long long>(hi - lo + 1));
    }
    bool chance(double p) { return uniform() < p; }

    // Poisson-distributed integer (Knuth's algorithm; fine for lambda < 15)
    int poisson(double lambda) {
        if (lambda <= 0) return 0;
        double L = std::exp(-lambda), p = 1.0;
        int k = 0;
        do { ++k; p *= uniform(); } while (p > L && k < 30);
        return k - 1;
    }

    // Fisher-Yates shuffle
    template <typename T>
    void shuffle(T* a, int n) {
        for (int i = n - 1; i > 0; --i) {
            int j = range(0, i);
            std::swap(a[i], a[j]);
        }
    }

private:
    unsigned long long state_ = 1;
};

}  // namespace ds
