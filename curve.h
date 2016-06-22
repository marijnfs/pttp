#ifndef __CURVE_H__
#define __CURVE_H__

#include <sodium.h>
#include <cassert>
#include "err.h"
#include "type.h"
 
struct Curve {
  
  Curve() {
    if (sodium_init() == -1)
      throw Err("Sodium Init Failed");
  }

  static Curve &inst() {
    if (!Curve::s_curve)
      Curve::s_curve = new Curve();
    return *Curve::s_curve;
  }

  static Curve *s_curve;

  bool bytes_equal(Bytes &b1, Bytes &b2) {
    if (b1.size() != b2.size()) return false;
    return sodium_memcmp(&b1[0], &b2[0], b1.size()) == 0; //constant time cmp to prevent sidechannel
  }

  int bytes_compare(Bytes &b1, Bytes b2) {
    assert(b1.size() == b2.size());
    return sodium_compare(&b1[0], &b2[0], b1.size());
  }

  void zero_bytes(Bytes &b) {
    sodium_memzero(&b[0], b.size());
  }

  void random_bytes(Bytes &b) {
    randombytes_buf(&b[0], b.size());
  }
  
  void increment(Bytes &b) {
    sodium_increment(&b[0], b.size());
  }
};

struct SafeBytes : public Bytes {
 SafeBytes(int n) : Bytes(n) {
  }
  
  ~SafeBytes() {
    sodium_memzero(&(*this)[0], size());
  }
};

struct PrivateKey : public SafeBytes {
 PrivateKey() : SafeBytes(crypto_box_SECRETKEYBYTES) {

 }

  SafeBytes pub() {
    SafeBytes pub_key(crypto_box_PUBLICKEYBYTES);
    crypto_scalarmult_base(&pub_key[0], &(*this)[0]);
    return pub_key;
  }
};

struct PublicKey : public SafeBytes  {
 PublicKey() : SafeBytes(crypto_box_PUBLICKEYBYTES) {

  }
};

struct Nonce : public SafeBytes {
 Nonce() : SafeBytes(crypto_box_NONCEBYTES) {
    reset();
  }

  void reset() {
    Curve::inst().random_bytes(*this);
  }

};

struct KeyPair {
  PrivateKey priv;
  PublicKey pub;
  
  KeyPair() {
    crypto_box_keypair(&pub[0], &priv[0]);
  }
};

struct EncryptedMessage {
  Bytes enc_message;
  
  EncryptedMessage(Bytes enc_message_, PublicKey &pub) : enc_message(enc_message_.size() + crypto_box_MACBYTES){
    
  } //init with an encrypted message, then decrypt
  
  EncryptedMessage(Bytes message, PrivateKey &priv) : enc_message(message) {
  
  } //directly encrypt a message

  Bytes decrypt(PrivateKey &priv) {

  }
  
};



#endif
