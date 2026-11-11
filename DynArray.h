#pragma once
// ============================================================================
//  DynArray<T> - a growable array (our replacement for std::vector).
//  Amortised O(1) push, O(1) random access, O(n) insert/remove in the middle.
// ============================================================================
#include <cassert>
#include <utility>

namespace ds {

template <typename T>
class DynArray {
public:
    DynArray() = default;
    explicit DynArray(int n) { resize(n); }
    DynArray(int n, const T& value) {
        resize(n);
        for (int i = 0; i < n; ++i) data_[i] = value;
    }
    DynArray(const DynArray& other) { copyFrom(other); }
    DynArray(DynArray&& other) noexcept { steal(other); }
    DynArray& operator=(const DynArray& other) {
        if (this != &other) { release(); copyFrom(other); }
        return *this;
    }
    DynArray& operator=(DynArray&& other) noexcept {
        if (this != &other) { release(); steal(other); }
        return *this;
    }
    ~DynArray() { release(); }

    // ---- capacity -------------------------------------------------------
    int size() const { return size_; }
    int capacity() const { return cap_; }
    bool empty() const { return size_ == 0; }

    void reserve(int capacity) {
        if (capacity <= cap_) return;
        T* fresh = new T[capacity];
        for (int i = 0; i < size_; ++i) fresh[i] = std::move(data_[i]);
        delete[] data_;
        data_ = fresh;
        cap_ = capacity;
    }
    void resize(int n) {
        reserve(n);
        for (int i = size_; i < n; ++i) data_[i] = T();
        size_ = n;
    }
    void clear() {
        for (int i = 0; i < size_; ++i) data_[i] = T();
        size_ = 0;
    }

    // ---- modifiers ------------------------------------------------------
    void push(const T& v) {
        T copy(v);                 // v may live inside this array
        grow();
        data_[size_++] = std::move(copy);
    }
    void push(T&& v) {
        grow();
        data_[size_++] = std::move(v);
    }
    void pop() {
        assert(size_ > 0);
        --size_;
        data_[size_] = T();
    }
    void insertAt(int idx, const T& v) {
        assert(idx >= 0 && idx <= size_);
        T copy(v);
        grow();
        for (int i = size_; i > idx; --i) data_[i] = std::move(data_[i - 1]);
        data_[idx] = std::move(copy);
        ++size_;
    }
    void removeAt(int idx) {
        assert(idx >= 0 && idx < size_);
        for (int i = idx; i + 1 < size_; ++i) data_[i] = std::move(data_[i + 1]);
        pop();
    }

    // ---- access ---------------------------------------------------------
    T& operator[](int i) { assert(i >= 0 && i < size_); return data_[i]; }
    const T& operator[](int i) const { assert(i >= 0 && i < size_); return data_[i]; }
    T& back() { assert(size_ > 0); return data_[size_ - 1]; }
    const T& back() const { assert(size_ > 0); return data_[size_ - 1]; }
    T* raw() { return data_; }
    const T* raw() const { return data_; }

    int indexOf(const T& v) const {
        for (int i = 0; i < size_; ++i)
            if (data_[i] == v) return i;
        return -1;
    }
    bool contains(const T& v) const { return indexOf(v) >= 0; }

    // range-for support
    T* begin() { return data_; }
    T* end() { return data_ + size_; }
    const T* begin() const { return data_; }
    const T* end() const { return data_ + size_; }

private:
    void grow() {
        if (size_ == cap_) reserve(cap_ == 0 ? 8 : cap_ * 2);
    }
    void copyFrom(const DynArray& o) {
        data_ = nullptr; size_ = 0; cap_ = 0;
        reserve(o.size_);
        for (int i = 0; i < o.size_; ++i) data_[i] = o.data_[i];
        size_ = o.size_;
    }
    void steal(DynArray& o) {
        data_ = o.data_; size_ = o.size_; cap_ = o.cap_;
        o.data_ = nullptr; o.size_ = 0; o.cap_ = 0;
    }
    void release() {
        delete[] data_;
        data_ = nullptr; size_ = 0; cap_ = 0;
    }

    T* data_ = nullptr;
    int size_ = 0;
    int cap_ = 0;
};

}  // namespace ds
