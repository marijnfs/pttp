#ifndef __CONVERT_H__
#define __CONVERT_H__

#include <string>
#include "type.h"
#include "err.h"

template <typename T>
inline T bytes_to_t(Bytes &b) {
  T t;
  std::string s(reinterpret_cast<char*>(&b[0]), b.size());
  if (!t.ParseFromString(s))
    throw Err("Could'nt parse from string");
  return t;
};


template <typename T>
inline Bytes t_to_bytes(T &t) {
  std::string buf;
  t.SerializeToString(&buf);
  Bytes bytes(buf.size());
  memcpy(&bytes[0], &buf[0], buf.size());
  return buf;
};

#endif
