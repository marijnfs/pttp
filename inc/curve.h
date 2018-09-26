#ifndef __CURVE_H__
#define __CURVE_H__

#include <sodium.h>
#include <cassert>
#include <iostream>
#include <string>

#include "err.h"
#include "type.h"
 
struct Curve {
  
  Curve();
  static Curve &inst();
  static Curve *s_curve;

  bool bytes_equal(Bytes &b1, Bytes &b2);

  int bytes_compare(Bytes &b1, Bytes b2);

  void zero_bytes(Bytes &b);

  void random_bytes(Bytes &b);
  
  void increment(Bytes &b);
};

inline void random_bytes(Bytes &b) {
  Curve::inst().random_bytes(b);
}

struct SafeBytes : public Bytes {
  SafeBytes();
  SafeBytes(Bytes &b);

  SafeBytes(int n);
  
  ~SafeBytes();

  void resize(size_t n);
};


struct SecretKey : public SafeBytes {
  SecretKey();

  SafeBytes pub();
};

struct PublicKey : public SafeBytes  {
  PublicKey();
};

struct SecretSignKey : public SafeBytes {
  SecretSignKey();

  //SafeBytes pub() {
  // SafeBytes pub_key(crypto_box_PUBLICKEYBYTES);
  //  crypto_scalarmult_base(&pub_key[0], &(*this)[0]);
  // return pub_key;
  //}
};

struct PublicSignKey : public SafeBytes  {
  PublicSignKey();
 PublicSignKey(Bytes &b) : ::SafeBytes(b) {}
};

struct Nonce : public SafeBytes {
  Nonce();

  void reset();

};

struct Seed : public SafeBytes {
  Seed();
  Seed(SafeBytes const &bytes);
};

struct HashKey : public SafeBytes {
  HashKey();
};

struct Hash : public Bytes {
  Hash(Bytes &m);
  Hash(Bytes &m, int nbytes);
  
  Hash(Bytes &m, HashKey &k);
};

struct HardHashSalt : public Bytes {
  HardHashSalt();
  HardHashSalt(Bytes &b);
};

const int HARD_HASH = 32;

struct HardHash : public Bytes {
  HardHash(Bytes &m, HardHashSalt &salt);
  HardHash(Bytes &m);  
};

struct HardestHash : public Bytes {
  HardestHash(Bytes &m, HardHashSalt &salt);
  HardestHash(Bytes &m);  
};

struct ShortHashKey : public Bytes {
 ShortHashKey(Bytes &b) : ::Bytes(b) {}
 ShortHashKey() : ::Bytes(crypto_shorthash_KEYBYTES) {
    Curve::inst().random_bytes(*this);
  }
};

struct ShortHash : public Bytes {
 ShortHash(ShortHashKey &key, Bytes &b) : ::Bytes(crypto_shorthash_BYTES) {    
    crypto_shorthash(ptr<unsigned char*>(), b.ptr<unsigned char *const>(), b.size(), key.ptr<unsigned char const*>());
  }

  void reset() {
    Curve::inst().random_bytes(*this);
  }

  uint64_t to_uint(int mod) {
    uint64_t val(*reinterpret_cast<uint64_t*>(&(*this)[0]));
    val >>= 32;
    val *= mod;
    val >>= 32;
    return val;
  }
};

struct KeyPair {
  SecretKey priv;
  PublicKey pub;
  
  KeyPair();
  KeyPair(Bytes seed);
};

struct SignKeyPair {
  SecretSignKey priv;
  PublicSignKey pub;
  
  SignKeyPair();
  SignKeyPair(Bytes seed);
};




struct EncryptedMessage {
  SafeBytes message, enc_message;
  Nonce nonce;
  
  EncryptedMessage(Bytes &message_, PublicKey &pub, SecretKey &priv);
  
  EncryptedMessage(Bytes &enc_message_);

  bool decrypt(PublicKey &pub, SecretKey &priv, Nonce &nonce);
  
};

struct SealedMessage {
  SafeBytes message, sealed_message;
  
  SealedMessage(Bytes &message_, PublicKey &pub);

  SealedMessage(Bytes &sealed_message_);
  
  bool decrypt(PublicKey &pub, SecretKey &priv);
  
};

struct SignedMessage {
  SafeBytes message;
  SafeBytes signed_message;
  
  SignedMessage(Bytes &message_, SecretSignKey &sk);

  SignedMessage(Bytes &signed_message_);

  bool verify(PublicSignKey &pub);
};

struct Signature {
  SafeBytes sig;

  Signature() {}
  Signature(Bytes &message_, SecretSignKey &sk);
  Signature(Bytes &signature_);

  bool verify(Bytes &message, PublicSignKey &pub);
};


inline Bytes hash_twin(Bytes* b1, Bytes *b2) {
  Bytes b(b1->size() + b2->size());
  copy(b1->begin(), b1->end(), b.begin());
  copy(b2->begin(), b2->end(), next(b.begin(), b1->size()));
  Hash h(b);
  return h;
}


inline Bytes hash_vec(std::vector<Bytes*> &hash_set) {
  assert(hash_set.size() > 0);

  std::vector<Bytes> hashes;
  
  for (int n(0); n < hash_set.size(); n += 2) {
    if (n == hash_set.size() - 1)
      hashes.push_back(hash_twin(hash_set[n], hash_set[n]));
    else
      hashes.push_back(hash_twin(hash_set[n], hash_set[n+1]));
  }

  while (hashes.size() > 1) {
    std::vector<Bytes> new_hashes;
    for (int n(0); n < hashes.size(); n += 2) {
      if (n == hashes.size() - 1)
	new_hashes.push_back(hash_twin(&hashes[n], &hashes[n]));
      else
	new_hashes.push_back(hash_twin(&hashes[n], &hashes[n+1]));
    }
    hashes = new_hashes;
  }

  return hashes[0];
}



#endif
