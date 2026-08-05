//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: no-localization
// XFAIL: FROZEN-CXX03-HEADERS-FIXME

// <random>

// template<class _IntType = int>
// class uniform_int_distribution

// template <class charT, class traits>
// basic_ostream<charT, traits>&
// operator<<(basic_ostream<charT, traits>& os,
//            const uniform_int_distribution& x);
//
// template <class charT, class traits>
// basic_istream<charT, traits>&
// operator>>(basic_istream<charT, traits>& is,
//            uniform_int_distribution& x);

#include <random>
#include <sstream>
#include <cassert>
#include <string>

#include "test_macros.h"

// A distribution whose IntType is a character type is written as a number, not as a character, and
// round-trips through both narrow and wide streams ([rand.req.genl], [rand.req.dist]).
template <class T>
void test_char_type(T a, T b, const char* expected) {
  typedef std::uniform_int_distribution<T> D;
  D d1(a, b);
  {
    std::ostringstream os;
    os << d1;
    assert(os.str() == expected);
    std::istringstream is(os.str());
    D d2;
    is >> d2;
    assert(!is.fail());
    assert(d1 == d2);
  }
#ifndef TEST_HAS_NO_WIDE_CHARACTERS
  {
    std::wostringstream os;
    os << d1;
    std::wistringstream is(os.str());
    D d2;
    is >> d2;
    assert(!is.fail());
    assert(d1 == d2);
  }
#endif
}

int main(int, char**)
{
    {
        typedef std::uniform_int_distribution<> D;
        D d1(3, 8);
        std::ostringstream os;
        os << d1;
        std::istringstream is(os.str());
        D d2;
        is >> d2;
        assert(d1 == d2);
    }
    {
      // Values that are whitespace codes, and negative values, would not survive being written
      // out as characters.
      test_char_type<signed char>(32, 100, "32 100");
      test_char_type<signed char>(9, 10, "9 10");
      test_char_type<signed char>(-128, 127, "-128 127");
      test_char_type<signed char>(-1, 0, "-1 0");
      test_char_type<unsigned char>(32, 100, "32 100");
      test_char_type<unsigned char>(0, 255, "0 255");
    }

  return 0;
}
