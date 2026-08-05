//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// <random>

// template<class IntType = int>
// class discrete_distribution

// template<class _URNG> result_type operator()(_URNG& g);

// IntType can be signed char or unsigned char ([rand.req.genl], P4037R1). The iterator constructor
// is used because the initializer_list one isn't available in C++03.

#include <random>
#include <cassert>
#include <cstddef>

#include "test_macros.h"

template <class T>
void test() {
  typedef std::discrete_distribution<T> D;
  std::mt19937 g;
  double p[] = {.3, .1, .6};
  D d(p, p + 3);
  assert(d.probabilities().size() == 3);
  assert(d.min() == 0);

  bool seen[3] = {};
  for (int i = 0; i < 10000; ++i) {
    typename D::result_type v = d(g);
    assert(0 <= v && v < 3);
    seen[static_cast<std::size_t>(v)] = true;
  }
  for (std::size_t i = 0; i < 3; ++i)
    assert(seen[i]);
}

int main(int, char**) {
  test<signed char>();
  test<unsigned char>();

  return 0;
}
