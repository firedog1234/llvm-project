//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// <random>

// template<class IntType = int>
// class geometric_distribution

// template<class _URNG> result_type operator()(_URNG& g);

// IntType can be signed char or unsigned char ([rand.req.genl], P4037R1). The probabilities below
// are chosen so that the distribution fits comfortably into a single byte; a geometric
// distribution is unbounded, so a small p would be clamped no matter how it is implemented.

#include <random>
#include <cassert>
#include <numeric>
#include <vector>

#include "test_macros.h"

template <class T>
void test(double p) {
  typedef std::geometric_distribution<T> D;
  std::mt19937 g;
  D d(p);
  assert(d.p() == p);
  assert(d.min() == 0);

  std::vector<double> u;
  for (int i = 0; i < 10000; ++i) {
    typename D::result_type v = d(g);
    assert(d.min() <= v && v <= d.max());
    u.push_back(static_cast<double>(v));
  }
  double mean          = std::accumulate(u.begin(), u.end(), 0.0) / u.size();
  double expected_mean = (1 - p) / p;
  assert(expected_mean * 0.85 < mean && mean < expected_mean * 1.15);
}

int main(int, char**) {
  test<signed char>(0.5);
  test<signed char>(0.75);
  test<unsigned char>(0.5);
  test<unsigned char>(0.25);

  return 0;
}
