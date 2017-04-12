#ifndef __UTIL_H__
#define __UTIL_H__

#include <vector>
#include <iostream>

template <typename T>
T &last(std::vector<T> &vec) {
  return vec[vec.size() - 1];
}

template <typename T>
std::ostream &operator<<(std::ostream &out, std::vector<T> &v) {
  if (v.size() == 0) return out << "[]";
  
  out << "[" << v[0];
  for (int i(1); i < v.size(); ++i)
    out << ", " << v[i];
  return out << "]";
}

#endif
