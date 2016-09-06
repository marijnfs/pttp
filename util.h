#ifndef __UTIL_H__
#define __UTIL_H__

#include <vector>

template <typename T>
T &last(std::vector<T> &vec) {
  return vec[vec.size() - 1];
}

#endif
