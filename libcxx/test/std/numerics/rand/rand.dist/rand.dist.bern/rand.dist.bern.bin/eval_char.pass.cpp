//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// XFAIL: FROZEN-CXX03-HEADERS-FIXME

// <random>

// template<class IntType = int>
// class binomial_distribution

// template<class _URNG> result_type operator()(_URNG& g);

// IntType can be signed char or unsigned char ([rand.req.genl], P4037R1). The values a binomial
// distribution produces are in [0, t], including when t is the largest value the result type can
// represent.

#include <random>
#include <cassert>

#include "test_macros.h"

template <class T>
void test(T t, double p) {
  typedef std::binomial_distribution<T> D;
  std::mt19937 g;
  D d(t, p);
  assert(d.t() == t);
  assert(d.min() == 0);
  assert(d.max() == t);
  for (int i = 0; i < 10000; ++i) {
    typename D::result_type v = d(g);
    assert(0 <= v && v <= t);
  }
}

int main(int, char**) {
  test<signed char>(127, 0.9);
  test<signed char>(127, 0.99);
  test<signed char>(127, 0.5);
  test<signed char>(100, 0.5);
  test<signed char>(1, 0.5);
  test<signed char>(0, 0.5);
  test<signed char>(127, 1);
  test<signed char>(127, 0);

  test<unsigned char>(255, 0.95);
  test<unsigned char>(255, 0.99);
  test<unsigned char>(255, 0.5);
  test<unsigned char>(1, 0.5);
  test<unsigned char>(0, 0.5);
  test<unsigned char>(255, 1);
  test<unsigned char>(255, 0);

  // The same overflow is reachable for wider result types; check the widest one that stays cheap.
  test<short>(32767, 0.99999);

  return 0;
}
