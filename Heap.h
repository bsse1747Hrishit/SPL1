#pragma once
// ============================================================================
//  Heap<T, Higher> - binary heap / priority queue (replacement for
//  std::priority_queue). `Higher(a, b)` returns true when `a` must sit above
//  `b` (closer to the root). Used for Top-N player leaderboards in
//  O(n log N) without sorting the whole population.
// ============================================================================
#include "DynArray.h"
#include <utility>

namespace ds {

template <typename T, typename Higher>
class Heap {
public:
    explicit Heap(Higher higher = Higher()) : higher_(higher) {}

    void push(const T& v) {
        a_.push(v);
        siftUp(a_.size() - 1);
    }
    T pop() {
        T top = a_[0];
        a_[0] = a_.back();
        a_.pop();
        if (!a_.empty()) siftDown(0);
        return top;
    }
    const T& top() const { return a_[0]; }
    int size() const { return a_.size(); }
    bool empty() const { return a_.empty(); }

private:
    void siftUp(int i) {
        while (i > 0) {
            int p = (i - 1) / 2;
            if (!higher_(a_[i], a_[p])) break;
            std::swap(a_[i], a_[p]);
            i = p;
        }
    }
    void siftDown(int i) {
        int n = a_.size();
        for (;;) {
            int l = 2 * i + 1, r = l + 1, best = i;
            if (l < n && higher_(a_[l], a_[best])) best = l;
            if (r < n && higher_(a_[r], a_[best])) best = r;
            if (best == i) break;
            std::swap(a_[i], a_[best]);
            i = best;
        }
    }

    DynArray<T> a_;
    Higher higher_;
};

}  // namespace ds
