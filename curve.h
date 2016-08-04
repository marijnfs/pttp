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

struct KeyPair {
  SecretKey priv;
  PublicKey pub;
  
  KeyPair();
};

struct SignKeyPair {
  SecretSignKey priv;
  PublicSignKey pub;
  
  SignKeyPair();
};



struct Hash {
  Bytes hash;
  
  Hash(Bytes &m);
  
  Hash(Bytes &m, HashKey &k);
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




std::ostream &operator<<(std::ostream &out, SafeBytes const &b);
#endif
