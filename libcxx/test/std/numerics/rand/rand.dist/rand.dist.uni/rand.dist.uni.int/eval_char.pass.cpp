//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// <random>

// template<class IntType = int>
// class uniform_int_distribution

// template<class _URNG> result_type operator()(_URNG& g);

// IntType can be signed char or unsigned char ([rand.req.genl], P4037R1).

#include <random>
#include <cassert>
#include <cstddef>

#include "test_macros.h"

template <class T>
void test_range(T a, T b) {
  typedef std::uniform_int_distribution<T> D;
  std::minstd_rand0 g;
  D d(a, b);
  assert(d.a() == a);
  assert(d.b() == b);
  assert(d.min() == a);
  assert(d.max() == b);
  for (int i = 0; i < 10000; ++i) {
    typename D::result_type v = d(g);
    assert(a <= v && v <= b);
  }
}

// Every value of a small range is produced, and no others.
template <class T>
void test_coverage(T a, T b) {
  std::minstd_rand0 g;
  std::uniform_int_distribution<T> d(a, b);
  const std::size_t n = static_cast<std::size_t>(b - a) + 1;
  bool seen[256]      = {};
  for (int i = 0; i < 10000; ++i) {
    typename std::uniform_int_distribution<T>::result_type v = d(g);
    assert(a <= v && v <= b);
    seen[static_cast<std::size_t>(v - a)] = true;
  }
  for (std::size_t i = 0; i < n; ++i)
    assert(seen[i]);
}

int main(int, char**) {
  test_range<signed char>(-128, 127);
  test_range<signed char>(0, 127);
  test_range<signed char>(-128, 0);
  test_range<unsigned char>(0, 255);

  test_coverage<signed char>(-5, 5);
  test_coverage<signed char>(-128, 127);
  test_coverage<unsigned char>(10, 20);
  test_coverage<unsigned char>(0, 255);

  // Degenerate ranges.
  test_range<signed char>(7, 7);
  test_range<unsigned char>(0, 0);

  return 0;
}
