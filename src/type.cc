#include "type.h"

#include <sodium.h>

using namespace std;

ostream &operator<<(ostream &out, Bytes const &b) {
  string hex(b.size() * 2 + 1, ' ');
  sodium_bin2hex(&hex[0], hex.size(), &b[0], b.size());
  return out << hex;
}
