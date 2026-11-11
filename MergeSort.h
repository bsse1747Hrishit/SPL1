#pragma once
// ============================================================================
//  mergeSort - stable O(n log n) merge sort (replacement for std::sort).
//  `before(a, b)` returns true when `a` must be placed before `b`.
//  Stability matters: the league table is sorted on several keys and equal
//  items must keep their relative order.
// ============================================================================
#include "DynArray.h"
#include <utility>

namespace ds {

namespace detail {
template <typename T, typename Before>
void mergeRec(T* a, T* tmp, int lo, int hi, Before& before) {
    if (hi - lo < 2) return;
    int mid = lo + (hi - lo) / 2;
    mergeRec(a, tmp, lo, mid, before);
    mergeRec(a, tmp, mid, hi, before);
    int i = lo, j = mid, k = lo;
    while (i < mid && j < hi) {
        // take from the right only if strictly before -> stable
        if (before(a[j], a[i])) tmp[k++] = a[j++];
        else tmp[k++] = a[i++];
    }
    while (i < mid) tmp[k++] = a[i++];
    while (j < hi) tmp[k++] = a[j++];
    for (int t = lo; t < hi; ++t) a[t] = std::move(tmp[t]);
}
}  // namespace detail

template <typename T, typename Before>
void mergeSort(T* a, int n, Before before) {
    if (n < 2) return;
    T* tmp = new T[n];
    detail::mergeRec(a, tmp, 0, n, before);
    delete[] tmp;
}

template <typename T, typename Before>
void mergeSort(DynArray<T>& arr, Before before) {
    mergeSort(arr.raw(), arr.size(), before);
}

}  // namespace ds
