#ifndef __BYTES_H__
#define __BYTES_H__

#include <vector>
#include <string.h>
#include <string>

#include <kj/common.h>
#include <kj/string.h>

#include <capnp/serialize.h>

//typedef std::vector<uint8_t> Bytes;

struct Bytes : public std::vector<uint8_t> {
  Bytes() {}
  Bytes(int n) : std::vector<uint8_t>(n) {}
 Bytes(std::string &s) : std::vector<uint8_t>(s.size()) { memcpy(&(*this)[0], &s[0], s.size()); }
 Bytes(const char *c) : std::vector<uint8_t>() { std::string s(c); resize(s.size()); memcpy(&(*this)[0], &s[0], s.size()); }

  template <typename T>
    Bytes(const T *c, size_t n) : std::vector<uint8_t>(reinterpret_cast<uint8_t const *>(c), reinterpret_cast<uint8_t const *>(c + n)) {};
 Bytes(unsigned char *b, unsigned char *e) : std::vector<uint8_t>(b, e) {}

  operator std::string() {
    std::string s(size(), 0);
    memcpy(&s[0], &(*this)[0], size());
    return s;
  }

  kj::ArrayPtr<kj::byte> kjp() {
    return kj::ArrayPtr<kj::byte>(&(*this)[0], size());
  }

  kj::ArrayPtr<::capnp::word const> kjwp() {
    return kj::ArrayPtr<::capnp::word const>((::capnp::word const*) &(*this)[0], (size()+1)/2);
  }
    
 template <typename T>
   T ptr() {
   return reinterpret_cast<T>(&(this[0]));
 }
  
  operator kj::ArrayPtr<kj::byte>() {
  //void bla() {
    return kj::ArrayPtr<kj::byte>(&(*this)[0], size());
  }
};

std::ostream &operator<<(std::ostream &out, Bytes const &b);

#endif
