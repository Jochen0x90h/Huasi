#pragma once

#include <string>
#include "optional.hpp"
#include "ptr.hpp"


/// cast string to integer. Returns null value if conversion is not possible
template <typename D, typename std::enable_if<std::is_integral<D>::value>::type* = nullptr>
optional<D> cast(std::string s) {
  using UD = typename std::make_unsigned<D>::type;

  D d = 0;
  if (!s.empty()) {
     auto it = s.begin();
     char sign = *it;

     // offset for overflow check
     UD offset = sign == '-' ? std::numeric_limits<D>::max() : std::numeric_limits<D>::min();

     UD value = 0;
     if (sign == '-' || sign == '+')
        ++it;
     for (; it != s.end(); ++it) {
        char ch = *it;

        // check if digit
        if (ch < '0' || ch > '9')
           return nullptr;

        UD nextValue = value * 10 + ch - '0';

        // check for overflow
        if (UD(offset + nextValue) <= value)
           return nullptr;

        value = nextValue;
     }

     d = sign == '-' ? -value : value;
  }
  return d;
}


/// cast integer to string. always succeeds
template <typename D, typename S, typename std::enable_if<std::is_integral<S>::value>::type* = nullptr>
D cast(S s) {
  char buffer[sizeof(s) * 3 + 2];

  typename std::make_unsigned<S>::type value = s < 0 ? -s : s;

  char * b = std::end(buffer) - 1;
  *b = 0;
  do {
     --b;
     *b = '0' + value % 10;
     value /= 10;
  } while (value > 0);

  if (s < 0) {
     --b;
     *b = '-';
  }

  return b;
}


/// cast pointer. Returns null pointer if conversion is not possible
template <typename D, typename S>
ptr<D> cast(ptr<S> s) {
  return dynamic_cast<D*>(s.p);
}
