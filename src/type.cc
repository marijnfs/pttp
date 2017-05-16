#include "type.h"

#include <sodium.h>
#include <iostream>

using namespace std;



ostream &operator<<(ostream &out, Bytes const &b) {
  string hex(b.size() * 2 + 1, ' ');
  sodium_bin2hex(&hex[0], hex.size(), &b[0], b.size());
  return out << hex;
}

void Bytes::from_hex(std::string hex) {
  resize(hex.size() / 2);
  sodium_hex2bin(&(*this)[0], size(), &hex[0], hex.size(), NULL, 0, 0);  
}
