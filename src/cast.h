#pragma once

#include <string>
#include "opt.h"
#include "ptr.h"


/// cast string to integer
template <typename D, typename std::enable_if<std::is_integral<D>::value>::type* = nullptr>
opt<D> cast(std::string s) {
  D d = 0;
  for (char ch : s) {
     D d10 = d * 10;

     // check if digit
     if (ch < '0' || ch > '9')
        return nullptr;

     // check for overflow
     if (d10 / 10 != d)
        return nullptr;

     d = d10 + ch - '0';
  }
  return d;
}


/// cast integer to string
template <typename D, typename S, typename std::enable_if<std::is_integral<S>::value>::type* = nullptr>
D cast(S s) {
  char buffer[sizeof(s) * 3 + 1];
  char * b = buffer + sizeof(s) * 3;
  *b = 0;
  do {
     --b;
     *b = '0' + s % 10;
     s /= 10;
  } while (s > 0);
  return b;
}


/// cast pointer
template <typename D, typename S>
ptr<D> cast(ptr<S> s) {
	return dynamic_cast<D*>(s.p);
}
