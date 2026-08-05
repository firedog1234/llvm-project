//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// <random>

// [rand.req.genl]
// A template argument corresponding to a template parameter named IntType may be any standard
// signed or standard unsigned integer type, which includes signed char and unsigned char.
//
// P4037R1 is applied as a defect report against C++11, so this holds in all language modes.

#include <random>
#include <type_traits>

#include "test_macros.h"

template <class T>
void test_distributions() {
  {
    typedef std::uniform_int_distribution<T> D;
    static_assert((std::is_same<typename D::result_type, T>::value), "");
    static_assert((std::is_same<typename D::param_type::distribution_type, D>::value), "");
    D d(T(1), T(4));
    D d2(typename D::param_type(T(1), T(4)));
    (void)(d == d2);
    (void)d.a();
    (void)d.b();
    (void)d.min();
    (void)d.max();
    d.param(d2.param());
  }
  {
    typedef std::binomial_distribution<T> D;
    static_assert((std::is_same<typename D::result_type, T>::value), "");
    static_assert((std::is_same<typename D::param_type::distribution_type, D>::value), "");
    D d(T(4), 0.5);
    (void)d.t();
    (void)d.p();
    (void)d.min();
    (void)d.max();
  }
  {
    typedef std::negative_binomial_distribution<T> D;
    static_assert((std::is_same<typename D::result_type, T>::value), "");
    static_assert((std::is_same<typename D::param_type::distribution_type, D>::value), "");
    D d(T(2), 0.75);
    (void)d.k();
    (void)d.p();
    (void)d.min();
    (void)d.max();
  }
  {
    typedef std::geometric_distribution<T> D;
    static_assert((std::is_same<typename D::result_type, T>::value), "");
    static_assert((std::is_same<typename D::param_type::distribution_type, D>::value), "");
    D d(0.5);
    (void)d.p();
    (void)d.min();
    (void)d.max();
  }
  {
    typedef std::poisson_distribution<T> D;
    static_assert((std::is_same<typename D::result_type, T>::value), "");
    static_assert((std::is_same<typename D::param_type::distribution_type, D>::value), "");
    D d(2.0);
    (void)d.mean();
    (void)d.min();
    (void)d.max();
  }
  {
    typedef std::discrete_distribution<T> D;
    static_assert((std::is_same<typename D::result_type, T>::value), "");
    static_assert((std::is_same<typename D::param_type::distribution_type, D>::value), "");
    double p[] = {.25, .25, .5};
    D d(p, p + 3);
    (void)d.probabilities();
    (void)d.min();
    (void)d.max();
  }
}

template <class T, class G>
void test_generation(G& g) {
  {
    std::uniform_int_distribution<T> d(T(1), T(4));
    static_assert((std::is_same<decltype(d(g)), T>::value), "");
    static_assert((std::is_same<decltype(d(g, d.param())), T>::value), "");
  }
  {
    std::binomial_distribution<T> d(T(4), 0.5);
    static_assert((std::is_same<decltype(d(g)), T>::value), "");
  }
  {
    std::negative_binomial_distribution<T> d(T(2), 0.75);
    static_assert((std::is_same<decltype(d(g)), T>::value), "");
  }
  {
    std::geometric_distribution<T> d(0.5);
    static_assert((std::is_same<decltype(d(g)), T>::value), "");
  }
  {
    std::poisson_distribution<T> d(2.0);
    static_assert((std::is_same<decltype(d(g)), T>::value), "");
  }
  {
    double p[] = {.25, .25, .5};
    std::discrete_distribution<T> d(p, p + 3);
    static_assert((std::is_same<decltype(d(g)), T>::value), "");
  }
}

void test() {
  test_distributions<signed char>();
  test_distributions<unsigned char>();

  std::minstd_rand0 g;
  test_generation<signed char>(g);
  test_generation<unsigned char>(g);
}
