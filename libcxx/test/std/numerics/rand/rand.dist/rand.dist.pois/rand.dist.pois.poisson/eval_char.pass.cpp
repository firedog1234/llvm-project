//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// <random>

// template<class IntType = int>
// class poisson_distribution

// template<class _URNG> result_type operator()(_URNG& g);

// IntType can be signed char or unsigned char ([rand.req.genl], P4037R1).

#include <random>
#include <cassert>
#include <numeric>
#include <vector>

#include "test_macros.h"

template <class T>
void test(double mean) {
  typedef std::poisson_distribution<T> D;
  std::mt19937 g;
  D d(mean);
  assert(d.mean() == mean);
  assert(d.min() == 0);

  std::vector<double> u;
  for (int i = 0; i < 10000; ++i) {
    typename D::result_type v = d(g);
    assert(d.min() <= v && v <= d.max());
    u.push_back(static_cast<double>(v));
  }
  double sample_mean = std::accumulate(u.begin(), u.end(), 0.0) / u.size();
  assert(mean * 0.9 < sample_mean && sample_mean < mean * 1.1);
}

int main(int, char**) {
  test<signed char>(0.75);
  test<signed char>(2);
  test<signed char>(20);
  test<unsigned char>(0.75);
  test<unsigned char>(2);
  test<unsigned char>(20);

  return 0;
}
