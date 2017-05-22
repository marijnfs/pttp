#ifndef __BLOOM_FILTER_H__
#define __BLOOM_FILTER_H__

#include <vector>
#include <algorithm>

#include "curve.h"

struct Bloom {
  int P;
  std::vector<bool> hits;
  ShortHashKey key;
  int n_on;
  
Bloom(int p) : P(p), hits(p), n_on(0)  {
}
  
Bloom(Bytes data, uint64_t p, ShortHashKey key_) : P(p), hits(p), key(key_), n_on(0) {
  int i(0);
  int n(0);
  int idx(0);
  for (bool bit : hits) {
    hits[i] = data[idx] & (1 << (7 - n));
        
    ++i;++n;
    if (n == 8) {
      ++idx;
      n = 0;
    }
  }

}
  
  void reset() {
    std::fill(hits.begin(), hits.end(), false);
    n_on = 0;
  }

  uint64_t hash(Bytes &b) {
    ShortHash h(key, b);
    return h.to_uint(P);
  }
  
  bool has(Bytes &b) {
    if (P == 0) return false;
    uint64_t val = hash(b);
    
    return hits[val];
  }

  bool set(Bytes &b) {
    uint64_t val = hash(b);
    if (!hits[val]) ++n_on;
    hits[val] = true;
  }

  bool has_set(Bytes &b) {
    uint64_t val = hash(b);
    bool has = hits[val];
    hits[val] = true;
    if (!has) ++n_on;
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


struct Cuckoo {
  //ShortHashKey h1, h2;
  Bloom filter1, filter2;
  vector<Bytes*> hashes, data;
  size_t N;
  
Cuckoo(size_t N_) : filter1(N_), filter2(N_), hashes(N_), N(N_) {
    
  }

  void store(Bytes hash, Bytes new_data) {
    int p = filter1.hash(hash);
    if (hashes[p]) {
      p = filter2.hash(hash);
      if (hashes[p]) {
	delete hashes[p];
	delete data[p];
      }
    }
    
    hashes[p] = new Bytes(hash);
    data[p] = new Bytes(new_data);
  }

  Bytes *get(Bytes hash) {
    int p = filter1.hash(hash);
    if (!hashes[p]) {
      p = filter2.hash(hash);
      if (!hashes[p])
	return 0;
      if (*hashes[p] != hash)
	return 0;
      return data[p];
    }
    if (*hashes[p] != hash)
      return 0;
    return data[p];
  }
  
  
};

#endif
