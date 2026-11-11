#pragma once
// ============================================================================
//  Queue<T> - FIFO queue on a circular buffer (replacement for std::queue).
//  Used for the fixture queue (which match is played next) and for the
//  breadth-first, round-by-round traversal of the knockout bracket tree.
// ============================================================================
#include <cassert>
#include <utility>

namespace ds {

template <typename T>
class Queue {
public:
    Queue() = default;
    Queue(const Queue& o) { copyFrom(o); }
    Queue& operator=(const Queue& o) {
        if (this != &o) { delete[] buf_; buf_ = nullptr; copyFrom(o); }
        return *this;
    }
    ~Queue() { delete[] buf_; }

    void enqueue(const T& v) {
        if (size_ == cap_) grow();
        buf_[(head_ + size_) % cap_] = v;
        ++size_;
    }
    T dequeue() {
        assert(size_ > 0);
        T v = std::move(buf_[head_]);
        head_ = (head_ + 1) % cap_;
        --size_;
        return v;
    }
    T& front() { assert(size_ > 0); return buf_[head_]; }
    const T& front() const { assert(size_ > 0); return buf_[head_]; }
    // i-th element from the front (0 = front), for previews of upcoming items.
    const T& peek(int i) const { assert(i >= 0 && i < size_); return buf_[(head_ + i) % cap_]; }

    int size() const { return size_; }
    bool empty() const { return size_ == 0; }
    void clear() { head_ = 0; size_ = 0; }

private:
    void grow() {
        int nc = cap_ == 0 ? 16 : cap_ * 2;
        T* nb = new T[nc];
        for (int i = 0; i < size_; ++i) nb[i] = std::move(buf_[(head_ + i) % cap_]);
        delete[] buf_;
        buf_ = nb;
        cap_ = nc;
        head_ = 0;
    }
    void copyFrom(const Queue& o) {
        cap_ = o.cap_; size_ = o.size_; head_ = 0;
        buf_ = cap_ ? new T[cap_] : nullptr;
        for (int i = 0; i < size_; ++i) buf_[i] = o.buf_[(o.head_ + i) % o.cap_];
    }

    T* buf_ = nullptr;
    int cap_ = 0;
    int head_ = 0;
    int size_ = 0;
};

}  // namespace ds
