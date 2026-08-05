//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___RANDOM_IS_VALID_H
#define _LIBCPP___RANDOM_IS_VALID_H

#include <__config>
#include <__type_traits/enable_if.h>
#include <__type_traits/integral_constant.h>
#include <__type_traits/is_same.h>
#include <__type_traits/is_unsigned.h>
#include <__utility/declval.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

// [rand.req.genl]/1.5:
// If a template argument corresponding to a template parameter named RealType is neither a
// standard floating-point type nor a member of an implementation-defined subset of extended
// floating-point types, the program is ill-formed. libc++'s subset of extended floating-point
// types is empty.

template <class>
struct __libcpp_random_is_valid_realtype : false_type {};
template <>
struct __libcpp_random_is_valid_realtype<float> : true_type {};
template <>
struct __libcpp_random_is_valid_realtype<double> : true_type {};
template <>
struct __libcpp_random_is_valid_realtype<long double> : true_type {};

// [rand.req.genl]/1.6:
// If a template argument corresponding to a template parameter named IntType is neither a standard
// signed nor a standard unsigned integer type, nor an extended integer type whose width is greater
// or equal to that of char and less than or equal to that of long long, nor a member of an
// implementation-defined subset of integer types, the program is ill-formed.
//
// signed char and unsigned char are required by P4037R1, which is applied here as a defect report
// against C++11 and is therefore not guarded by a language mode check. char, bool, wchar_t and the
// charN_t types are integer types too, so an implementation is allowed to accept them, but they are
// not part of libc++'s implementation-defined subset. That subset consists of the 128-bit integer
// types, whose width is greater than that of long long.

template <class>
struct __libcpp_random_is_valid_inttype : false_type {};
template <>
struct __libcpp_random_is_valid_inttype<signed char> : true_type {};
template <>
struct __libcpp_random_is_valid_inttype<short> : true_type {};
template <>
struct __libcpp_random_is_valid_inttype<int> : true_type {};
template <>
struct __libcpp_random_is_valid_inttype<long> : true_type {};
template <>
struct __libcpp_random_is_valid_inttype<long long> : true_type {};
template <>
struct __libcpp_random_is_valid_inttype<unsigned char> : true_type {};
template <>
struct __libcpp_random_is_valid_inttype<unsigned short> : true_type {};
template <>
struct __libcpp_random_is_valid_inttype<unsigned int> : true_type {};
template <>
struct __libcpp_random_is_valid_inttype<unsigned long> : true_type {};
template <>
struct __libcpp_random_is_valid_inttype<unsigned long long> : true_type {};

#if _LIBCPP_HAS_INT128
template <>
struct __libcpp_random_is_valid_inttype<__int128_t> : true_type {}; // extension
template <>
struct __libcpp_random_is_valid_inttype<__uint128_t> : true_type {}; // extension
#endif                                                               // _LIBCPP_HAS_INT128

// The type a distribution streams an IntType value through. signed char and unsigned char have to
// be widened first: inserting them would write a character instead of a number, and there is no
// extractor for them at all when the stream's character type isn't char, so [rand.req.dist]'s
// requirement that a distribution be restorable from its textual representation could not be met.

template <class _IntType>
struct __libcpp_random_stream_type {
  typedef _IntType type;
};
template <>
struct __libcpp_random_stream_type<signed char> {
  typedef int type;
};
template <>
struct __libcpp_random_stream_type<unsigned char> {
  typedef unsigned int type;
};

// [rand.req.urng]/3:
// A class G meets the uniform random bit generator requirements if G models
// uniform_random_bit_generator, invoke_result_t<G&> is an unsigned integer type,
// and G provides a nested typedef-name result_type that denotes the same type
// as invoke_result_t<G&>.
// (In particular, reject URNGs with signed result_types; our distributions cannot
// handle such generator types.)

template <class, class = void>
struct __libcpp_random_is_valid_urng : false_type {};
template <class _Gp>
struct __libcpp_random_is_valid_urng<
    _Gp,
    __enable_if_t< is_unsigned<typename _Gp::result_type>::value &&
                   _IsSame<decltype(std::declval<_Gp&>()()), typename _Gp::result_type>::value > > : true_type {};

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___RANDOM_IS_VALID_H
