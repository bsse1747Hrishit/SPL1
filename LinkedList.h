#pragma once
// ============================================================================
//  LinkedList<T> - doubly linked list (replacement for std::list).
//  Used for the chronological event log of every match and penalty kicks.
//  O(1) push/pop at both ends, O(n) indexed access.
// ============================================================================
#include <cassert>

namespace ds {

template <typename T>
class LinkedList {
    struct Node {
        T value;
        Node* prev;
        Node* next;
    };

public:
    class Iterator {
    public:
        explicit Iterator(Node* n) : n_(n) {}
        T& operator*() const { return n_->value; }
        T* operator->() const { return &n_->value; }
        Iterator& operator++() { n_ = n_->next; return *this; }
        bool operator!=(const Iterator& o) const { return n_ != o.n_; }
    private:
        Node* n_;
    };
    class ConstIterator {
    public:
        explicit ConstIterator(const Node* n) : n_(n) {}
        const T& operator*() const { return n_->value; }
        const T* operator->() const { return &n_->value; }
        ConstIterator& operator++() { n_ = n_->next; return *this; }
        bool operator!=(const ConstIterator& o) const { return n_ != o.n_; }
    private:
        const Node* n_;
    };

    LinkedList() = default;
    LinkedList(const LinkedList& o) { for (const T& v : o) pushBack(v); }
    LinkedList(LinkedList&& o) noexcept : head_(o.head_), tail_(o.tail_), size_(o.size_) {
        o.head_ = o.tail_ = nullptr;
        o.size_ = 0;
    }
    LinkedList& operator=(const LinkedList& o) {
        if (this != &o) { clear(); for (const T& v : o) pushBack(v); }
        return *this;
    }
    LinkedList& operator=(LinkedList&& o) noexcept {
        if (this != &o) {
            clear();
            head_ = o.head_; tail_ = o.tail_; size_ = o.size_;
            o.head_ = o.tail_ = nullptr; o.size_ = 0;
        }
        return *this;
    }
    ~LinkedList() { clear(); }

    void pushBack(const T& v) {
        Node* n = new Node{v, tail_, nullptr};
        if (tail_) tail_->next = n; else head_ = n;
        tail_ = n;
        ++size_;
    }
    void pushFront(const T& v) {
        Node* n = new Node{v, nullptr, head_};
        if (head_) head_->prev = n; else tail_ = n;
        head_ = n;
        ++size_;
    }
    void popFront() {
        assert(head_);
        Node* n = head_;
        head_ = n->next;
        if (head_) head_->prev = nullptr; else tail_ = nullptr;
        delete n;
        --size_;
    }
    void popBack() {
        assert(tail_);
        Node* n = tail_;
        tail_ = n->prev;
        if (tail_) tail_->next = nullptr; else head_ = nullptr;
        delete n;
        --size_;
    }
    void clear() {
        while (head_) popFront();
    }

    T& front() { assert(head_); return head_->value; }
    T& back() { assert(tail_); return tail_->value; }
    const T& front() const { assert(head_); return head_->value; }
    const T& back() const { assert(tail_); return tail_->value; }

    int size() const { return size_; }
    bool empty() const { return size_ == 0; }

    Iterator begin() { return Iterator(head_); }
    Iterator end() { return Iterator(nullptr); }
    ConstIterator begin() const { return ConstIterator(head_); }
    ConstIterator end() const { return ConstIterator(nullptr); }

private:
    Node* head_ = nullptr;
    Node* tail_ = nullptr;
    int size_ = 0;
};

}  // namespace ds
