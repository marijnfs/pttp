#ifndef __BLOOM_FILTER_H__
#define __BLOOM_FILTER_H__

#include <vector>
#include <algorithm>

#include "curve.h"

struct Bloom {
  int P;
  std::vector<bool> hits;
  ShortHashKey key;
  
Bloom(int p) : P(p), hits(p)  {
}
  
Bloom(Bytes data, uint64_t p, ShortHashKey key_) : P(p), hits(p), key(key_) {
  int i(0);
  int n(0);
  int idx(0);
  for (bool bit : hits) {
    cout << int(data[idx]) << endl;
    cout << int(data[idx]) << " " << (1 << (7 - n)) <<  " " << (int(data[idx]) & (1 << (7 - n))) << endl;
    hits[i] = data[idx] & (1 << (7 - n));
    cout << hits[i] << endl;
    
    ++i;++n;
    if (n == 8) {
      ++idx;
      n = 0;
    }
  }

}
  
  void reset() {
    std::fill(hits.begin(), hits.end(), false);
  }

  uint64_t hash(Bytes &b) {
    ShortHash h(key, b);
    uint64_t val(*reinterpret_cast<uint64_t*>(&h[0]));
    val >>= 32;
    val *= P;
    val >>= 32;
    return val;
  }
  
  bool has(Bytes &b) {
    uint64_t val = hash(b);
    
    return hits[val];
  }

  bool set(Bytes &b) {
    uint64_t val = hash(b);
    hits[val] = true;
  }

  bool has_set(Bytes &b) {
    uint64_t val = hash(b);
    bool has = hits[val];
    hits[val] = true;
    return has;
  }

  Bytes bytes() {
    Bytes b((P + 7) / 8);

    int i(0);
    int n(0);
    for (bool bit : hits) {
      b[i] <<= 1;
      b[i] |= bit;
      ++n;
      if (n == 8) {
	++i;
	n = 0;
      }
    }
    b[b.size()-1] <<= 8 - n;
    return b;
  }
  
};

#endif
