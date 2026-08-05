//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// <random>

// Which types are accepted for a template parameter named IntType. The standard signed and
// standard unsigned integer types are required by [rand.req.genl]; everything else here is
// libc++'s choice of implementation-defined subset.

#include <random>

#include "test_macros.h"

// Required by [rand.req.genl].
static_assert(std::__libcpp_random_is_valid_inttype<signed char>::value, "");
static_assert(std::__libcpp_random_is_valid_inttype<short>::value, "");
static_assert(std::__libcpp_random_is_valid_inttype<int>::value, "");
static_assert(std::__libcpp_random_is_valid_inttype<long>::value, "");
static_assert(std::__libcpp_random_is_valid_inttype<long long>::value, "");
static_assert(std::__libcpp_random_is_valid_inttype<unsigned char>::value, "");
static_assert(std::__libcpp_random_is_valid_inttype<unsigned short>::value, "");
static_assert(std::__libcpp_random_is_valid_inttype<unsigned int>::value, "");
static_assert(std::__libcpp_random_is_valid_inttype<unsigned long>::value, "");
static_assert(std::__libcpp_random_is_valid_inttype<unsigned long long>::value, "");

// The implementation-defined subset: 128 bit integers, whose width is greater than that of
// long long.
#ifndef TEST_HAS_NO_INT128
static_assert(std::__libcpp_random_is_valid_inttype<__int128_t>::value, "");
static_assert(std::__libcpp_random_is_valid_inttype<__uint128_t>::value, "");
#endif

// [rand.req.genl] would allow these as part of the implementation-defined subset of integer types,
// but libc++ doesn't support them. In particular, char is a distinct type from both signed char and
// unsigned char, and stays unsupported.
static_assert(!std::__libcpp_random_is_valid_inttype<char>::value, "");
static_assert(!std::__libcpp_random_is_valid_inttype<bool>::value, "");
#ifndef TEST_HAS_NO_WIDE_CHARACTERS
static_assert(!std::__libcpp_random_is_valid_inttype<wchar_t>::value, "");
#endif
#if TEST_STD_VER >= 11
static_assert(!std::__libcpp_random_is_valid_inttype<char16_t>::value, "");
static_assert(!std::__libcpp_random_is_valid_inttype<char32_t>::value, "");
#endif
#if TEST_STD_VER >= 20
static_assert(!std::__libcpp_random_is_valid_inttype<char8_t>::value, "");
#endif

// cv-qualified arguments are ill-formed.
static_assert(!std::__libcpp_random_is_valid_inttype<const int>::value, "");
static_assert(!std::__libcpp_random_is_valid_inttype<volatile int>::value, "");
static_assert(!std::__libcpp_random_is_valid_inttype<const volatile unsigned char>::value, "");

static_assert(!std::__libcpp_random_is_valid_inttype<float>::value, "");
static_assert(!std::__libcpp_random_is_valid_inttype<int*>::value, "");
static_assert(!std::__libcpp_random_is_valid_inttype<int&>::value, "");
