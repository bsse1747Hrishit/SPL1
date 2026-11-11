#pragma once
// ============================================================================
//  HashMap<K,V> - hash table with separate chaining (replacement for
//  std::unordered_map). Rehashes when the load factor exceeds 0.75.
//  Used for: team lookup by code, player lookup by name, head-to-head
//  records, country statistics and forced "what-if" results.
// ============================================================================
#include <string>
#include "DynArray.h"

namespace ds {

// ---- hash functions ------------------------------------------------------
inline unsigned long long hashOf(const std::string& s) {       // FNV-1a
    unsigned long long h = 1469598103934665603ULL;
    for (unsigned char c : s) { h ^= c; h *= 1099511628211ULL; }
    return h;
}
inline unsigned long long hashOf(long long k) {                // splitmix64
    unsigned long long x = static_cast<unsigned long long>(k) + 0x9E3779B97F4A7C15ULL;
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
    return x ^ (x >> 31);
}
inline unsigned long long hashOf(int k) { return hashOf(static_cast<long long>(k)); }

template <typename K, typename V>
class HashMap {
    struct Entry {
        K key;
        V value;
        Entry* next;
    };

public:
    explicit HashMap(int initialBuckets = 16) { init(initialBuckets); }
    HashMap(const HashMap& o) {
        init(o.bucketCount_);
        o.forEach([this](const K& k, const V& v) { put(k, v); });
    }
    HashMap& operator=(const HashMap& o) {
        if (this != &o) {
            destroy();
            init(o.bucketCount_);
            o.forEach([this](const K& k, const V& v) { put(k, v); });
        }
        return *this;
    }
    ~HashMap() { destroy(); }

    // Insert or overwrite.
    void put(const K& key, const V& value) {
        Entry* e = findEntry(key);
        if (e) { e->value = value; return; }
        if ((size_ + 1) * 4 > bucketCount_ * 3) rehash(bucketCount_ * 2);
        int b = bucketFor(key);
        buckets_[b] = new Entry{key, value, buckets_[b]};
        ++size_;
    }
    // Returns nullptr when the key is absent.
    V* find(const K& key) {
        Entry* e = findEntry(key);
        return e ? &e->value : nullptr;
    }
    const V* find(const K& key) const {
        const Entry* e = const_cast<HashMap*>(this)->findEntry(key);
        return e ? &e->value : nullptr;
    }
    bool contains(const K& key) const { return find(key) != nullptr; }

    // Access, inserting a default value when absent.
    V& operator[](const K& key) {
        Entry* e = findEntry(key);
        if (!e) { put(key, V()); e = findEntry(key); }
        return e->value;
    }
    bool remove(const K& key) {
        int b = bucketFor(key);
        Entry* prev = nullptr;
        for (Entry* e = buckets_[b]; e; prev = e, e = e->next) {
            if (e->key == key) {
                if (prev) prev->next = e->next; else buckets_[b] = e->next;
                delete e;
                --size_;
                return true;
            }
        }
        return false;
    }
    void clear() {
        destroy();
        init(16);
    }

    template <typename F>
    void forEach(F f) const {
        for (int b = 0; b < bucketCount_; ++b)
            for (Entry* e = buckets_[b]; e; e = e->next) f(e->key, e->value);
    }
    DynArray<K> keys() const {
        DynArray<K> out;
        forEach([&out](const K& k, const V&) { out.push(k); });
        return out;
    }

    int size() const { return size_; }
    int bucketCount() const { return bucketCount_; }
    double loadFactor() const { return bucketCount_ ? double(size_) / bucketCount_ : 0.0; }
    // Length of the longest chain - a quick quality indicator of the hash.
    int longestChain() const {
        int best = 0;
        for (int b = 0; b < bucketCount_; ++b) {
            int len = 0;
            for (Entry* e = buckets_[b]; e; e = e->next) ++len;
            if (len > best) best = len;
        }
        return best;
    }

private:
    void init(int n) {
        if (n < 4) n = 4;
        bucketCount_ = n;
        size_ = 0;
        buckets_ = new Entry*[n];
        for (int i = 0; i < n; ++i) buckets_[i] = nullptr;
    }
    void destroy() {
        for (int b = 0; b < bucketCount_; ++b) {
            Entry* e = buckets_[b];
            while (e) { Entry* nx = e->next; delete e; e = nx; }
        }
        delete[] buckets_;
        buckets_ = nullptr;
        bucketCount_ = 0;
        size_ = 0;
    }
    int bucketFor(const K& key) const {
        return static_cast<int>(hashOf(key) % static_cast<unsigned long long>(bucketCount_));
    }
    Entry* findEntry(const K& key) {
        for (Entry* e = buckets_[bucketFor(key)]; e; e = e->next)
            if (e->key == key) return e;
        return nullptr;
    }
    void rehash(int newCount) {
        Entry** old = buckets_;
        int oldCount = bucketCount_;
        buckets_ = new Entry*[newCount];
        for (int i = 0; i < newCount; ++i) buckets_[i] = nullptr;
        bucketCount_ = newCount;
        for (int b = 0; b < oldCount; ++b) {
            Entry* e = old[b];
            while (e) {
                Entry* nx = e->next;
                int nb = bucketFor(e->key);
                e->next = buckets_[nb];
                buckets_[nb] = e;
                e = nx;
            }
        }
        delete[] old;
    }

    Entry** buckets_ = nullptr;
    int bucketCount_ = 0;
    int size_ = 0;
};

}  // namespace ds
