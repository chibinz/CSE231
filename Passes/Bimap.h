#pragma once

#include <map>

template <typename U, typename V> class Bimap {
private:
  std::map<U, V> A2BMap;
  std::map<V, U> B2AMap;

public:
  auto insert(U a, V b) { insert(b, a); }

  auto insert(V b, U a) {
    A2BMap.insert({a, b});
    B2AMap.insert({b, a});
  }

  auto operator[](U a) { return A2BMap[a]; }

  auto operator[](V b) { return B2AMap[b]; }
};