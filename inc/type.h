#ifndef __BYTES_H__
#define __BYTES_H__

#include <vector>
#include <string>
#include <cassert>
#include <bitset>
#include <iostream>

#include <kj/common.h>
#include <kj/string.h>

#include <capnp/serialize.h>

#include "messages.capnp.h"

//typedef std::vector<uint8_t> Bytes;

struct Bytes : public std::vector<uint8_t> {
  Bytes() {}
  Bytes(int n) : std::vector<uint8_t>(n) {}
 Bytes(std::string &s) : std::vector<uint8_t>(s.size()) { memcpy(&(*this)[0], &s[0], s.size()); }
 Bytes(const char *c) : std::vector<uint8_t>() { std::string s(c); resize(s.size()); memcpy(&(*this)[0], &s[0], s.size()); }
  //Bytes(capnp::Data::Reader r): ::Bytes(r.begin(), r.end()) {}

 Bytes(capnp::Data::Reader &r): ::Bytes(r.begin(), r.end()) {}
  
  template <long unsigned int N>
 Bytes(std::bitset<N> &bits) : std::vector<uint8_t>((bits.size()+1)/8) {
    int idx(0);
    int n(0);
    uint8_t u(0);
    
    for (int i(0); i < bits.size(); ++i) {
      u = (u << 1) | bits[i];
      ++n;
      if (n == 8) {
	(*this)[idx++] = u;
	u = 0;
	n = 0;
      }
    }
    if (idx < size()) (*this)[idx] = u;
  }
  
  Bytes &operator=(capnp::Data::Reader const &r) {*this = Bytes(r.begin(), r.end()); return *this;}
  
  template <typename T>
    Bytes(T *c, T *e) : std::vector<uint8_t>(c, e) {}

  template <typename T>
    Bytes(const T *c, size_t n) : std::vector<uint8_t>(reinterpret_cast<uint8_t const *>(c), reinterpret_cast<uint8_t const *>(c + n)) {};
 Bytes(unsigned char *b, unsigned char *e) : std::vector<uint8_t>(b, e) {}

  void zero() {std::fill(begin(), end(), 0);}


  template <long unsigned int N>
    std::bitset<N> bitset() {
    std::bitset<N> bits;
    int idx(0);
    int n(0);
    uint8_t u(0);
    
    for (int i(0); i < bits.size(); ++i) {
      int base = 8;
      if ((i / 8) == (bits.size() / 8))
	base = bits.size() % 8;
      uint8_t mask(1 << (base - 1 - n));
      std::cout << int((*this)[idx]) << " " << int(mask) << " " << (int((*this)[idx]) & int(mask)) << std::endl;
      bits[i] = (*this)[idx] & mask;
      ++n;
      if (n == 8) {
	idx++;
	n = 0;
      }
    }
    return bits;
  }



  
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
   return reinterpret_cast<T>(&(*this)[0]);
 }

 void x_or(Bytes &b) {
   assert(b.size() == size());
   for (size_t i(0); i < size(); ++i)
     (*this)[i] ^= b[i];
 }

 void from_hex(std::string hex);
 
  operator kj::ArrayPtr<kj::byte>() {
  //void bla() {
    return kj::ArrayPtr<kj::byte>(&(*this)[0], size());
  }
};

std::ostream &operator<<(std::ostream &out, Bytes const &b);

#endif
