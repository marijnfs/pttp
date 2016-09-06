#ifndef __BYTES_H__
#define __BYTES_H__

#include <vector>
#include <cstdint>
#include <string>

//typedef std::vector<uint8_t> Bytes;

struct Bytes : public std::vector<uint8_t> {
  Bytes() {}
  Bytes(int n) : std::vector<uint8_t>(n) {}
 Bytes(std::string &s) : std::vector<uint8_t>(s.size()) { memcpy(&(*this)[0], &s[0], s.size()); }
 Bytes(const char *c) : std::vector<uint8_t>() { std::string s(c); resize(s.size()); memcpy(&(*this)[0], &s[0], s.size()); }

  operator std::string() {
    std::string s(size(), 0);
    memcpy(&s[0], &(*this)[0], size());
    return s;
  }
};

std::ostream &operator<<(std::ostream &out, Bytes const &b);

#endif
