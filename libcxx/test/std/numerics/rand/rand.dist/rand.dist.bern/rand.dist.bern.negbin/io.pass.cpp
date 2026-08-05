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

// template<class IntType = int>
// class negative_binomial_distribution

// template <class charT, class traits>
// basic_ostream<charT, traits>&
// operator<<(basic_ostream<charT, traits>& os,
//            const negative_binomial_distribution& x);
//
// template <class charT, class traits>
// basic_istream<charT, traits>&
// operator>>(basic_istream<charT, traits>& is,
//            negative_binomial_distribution& x);

#include <random>
#include <sstream>
#include <cassert>
#include <string>

#include "test_macros.h"

// A distribution whose IntType is a character type is written as a number, not as a character, and
// round-trips through both narrow and wide streams ([rand.req.genl], [rand.req.dist]).
template <class T>
void test_char_type(T k, const char* expected) {
  typedef std::negative_binomial_distribution<T> D;
  D d1(k, .25);
  {
    std::ostringstream os;
    os << d1;
    assert(os.str().substr(0, os.str().find(' ')) == expected);
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
        typedef std::negative_binomial_distribution<> D;
        D d1(7, .25);
        std::ostringstream os;
        os << d1;
        std::istringstream is(os.str());
        D d2;
        is >> d2;
        assert(d1 == d2);
    }
    {
      // A value that is a whitespace code would not survive being written out as a character.
      test_char_type<signed char>(32, "32");
      test_char_type<signed char>(10, "10");
      test_char_type<signed char>(127, "127");
      test_char_type<unsigned char>(32, "32");
      test_char_type<unsigned char>(255, "255");
    }

  return 0;
}
