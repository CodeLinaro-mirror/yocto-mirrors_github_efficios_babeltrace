/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (C) 2026 EfficiOS, Inc.
 */

#include <functional>
#include <vector>

#include "cpp-common/bt2c/prio-heap.hpp"

#include "catch2/catch_test_macros.hpp"

TEST_CASE("Default-constructed `bt2c::PrioHeap` is empty")
{
    const bt2c::PrioHeap<int> heap;

    CHECK(heap.isEmpty());
    CHECK(heap.len() == 0);
}

TEST_CASE("bt2c::PrioHeap::insert() increases the length")
{
    bt2c::PrioHeap<int> heap;

    heap.insert(1);
    CHECK(heap.len() == 1);
    CHECK_FALSE(heap.isEmpty());
    heap.insert(2);
    CHECK(heap.len() == 2);
}

TEST_CASE("bt2c::PrioHeap::top() with the default comparator returns the greatest element")
{
    bt2c::PrioHeap<int> heap;

    heap.insert(5);
    heap.insert(1);
    heap.insert(9);
    heap.insert(3);
    CHECK(heap.top() == 9);
}

TEST_CASE("bt2c::PrioHeap::removeTop() removes elements in descending order")
{
    bt2c::PrioHeap<int> heap;

    for (const auto val : std::array {5, 1, 9, 3, 7, 2, 8}) {
        heap.insert(val);
    }

    std::vector<int> sorted;

    while (!heap.isEmpty()) {
        sorted.push_back(heap.top());
        heap.removeTop();
    }

    CHECK(sorted == std::vector {9, 8, 7, 5, 3, 2, 1});
    CHECK(heap.isEmpty());
}

TEST_CASE("bt2c::PrioHeap::removeTop() with a single element empties the heap")
{
    bt2c::PrioHeap<int> heap;

    heap.insert(42);
    heap.removeTop();
    CHECK(heap.isEmpty());
}

TEST_CASE("bt2c::PrioHeap::replaceTop() replaces the top element and rebalances")
{
    bt2c::PrioHeap<int> heap;

    heap.insert(5);
    heap.insert(1);
    heap.insert(9);
    CHECK(heap.top() == 9);
    heap.replaceTop(2);
    CHECK(heap.len() == 3);
    CHECK(heap.top() == 5);
}

TEST_CASE("bt2c::PrioHeap::clear() empties the heap")
{
    bt2c::PrioHeap<int> heap;

    heap.insert(1);
    heap.insert(2);
    heap.clear();
    CHECK(heap.isEmpty());
    CHECK(heap.len() == 0);
}

TEST_CASE("`bt2c::PrioHeap` with a custom comparator behaves as a min-heap")
{
    bt2c::PrioHeap<int, std::greater<>> maxHeap;
    bt2c::PrioHeap<int, std::less<>> minHeap;

    for (const auto val : std::array {5, 1, 9, 3, 7}) {
        maxHeap.insert(val);
        minHeap.insert(val);
    }

    CHECK(maxHeap.top() == 9);
    CHECK(minHeap.top() == 1);
}

TEST_CASE("`bt2c::PrioHeap` constructed with an initial capacity is empty")
{
    CHECK(bt2c::PrioHeap<int> {std::greater<int> {}, 16}.isEmpty());
}
