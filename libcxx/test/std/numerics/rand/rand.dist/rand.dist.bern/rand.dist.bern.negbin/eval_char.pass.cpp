//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// <random>

// template<class IntType = int>
// class negative_binomial_distribution

// template<class _URNG> result_type operator()(_URNG& g);

// IntType can be signed char or unsigned char ([rand.req.genl], P4037R1). The parameters below are
// chosen so that the distribution fits comfortably into a single byte; a negative binomial
// distribution is unbounded, so parameters whose typical values exceed the result type's range
// would be clamped no matter how the distribution is implemented.

#include <random>
#include <cassert>
#include <numeric>
#include <vector>

#include "test_macros.h"

template <class T>
void test(T k, double p, double expected_mean) {
  typedef std::negative_binomial_distribution<T> D;
  std::mt19937 g;
  D d(k, p);
  assert(d.k() == k);
  assert(d.p() == p);
  assert(d.min() == 0);

  std::vector<double> u;
  for (int i = 0; i < 10000; ++i) {
    typename D::result_type v = d(g);
    assert(d.min() <= v && v <= d.max());
    u.push_back(static_cast<double>(v));
  }
  double mean = std::accumulate(u.begin(), u.end(), 0.0) / u.size();
  assert(expected_mean * 0.9 < mean && mean < expected_mean * 1.1);
}

int main(int, char**) {
  test<signed char>(3, 0.75, 1.0);   // mean k(1-p)/p == 1
  test<signed char>(1, 0.5, 1.0);    // mean == 1
  test<unsigned char>(3, 0.75, 1.0); // mean == 1
  test<unsigned char>(5, 0.5, 5.0);  // mean == 5

  return 0;
}
