#include "curve.h"

using namespace std;

Curve *Curve::s_curve = 0;


Curve::Curve() {
  if (sodium_init() == -1)
    throw Err("Sodium Init Failed");
}

Curve &Curve::inst() {
  if (!Curve::s_curve)
    Curve::s_curve = new Curve();
  return *Curve::s_curve;
}

bool Curve::bytes_equal(Bytes &b1, Bytes &b2) {
  if (b1.size() != b2.size()) return false;
  return sodium_memcmp(&b1[0], &b2[0], b1.size()) == 0; //constant time cmp to prevent sidechannel
}

int Curve::bytes_compare(Bytes &b1, Bytes b2) {
  assert(b1.size() == b2.size());
  return sodium_compare(&b1[0], &b2[0], b1.size());
}

void Curve::zero_bytes(Bytes &b) {
  sodium_memzero(&b[0], b.size());
}

void Curve::random_bytes(Bytes &b) {
  randombytes_buf(&b[0], b.size());
}
  
void Curve::increment(Bytes &b) {
  sodium_increment(&b[0], b.size());
}



SafeBytes::SafeBytes() : Bytes() {}    
SafeBytes::SafeBytes(Bytes &b) : Bytes(b) {}

SafeBytes::SafeBytes(int n) : Bytes(n) {
}
  
SafeBytes::~SafeBytes() {
  sodium_memzero(&(*this)[0], size());
}

void SafeBytes::resize(size_t n) {
  if (size() != 0)
    sodium_memzero(&(*this)[0], size());
  Bytes::resize(n);
}



SecretKey::SecretKey() : SafeBytes(crypto_box_SECRETKEYBYTES) {
  
}

SafeBytes SecretKey::pub() {
  SafeBytes pub_key(crypto_box_PUBLICKEYBYTES);
  crypto_scalarmult_base(&pub_key[0], &(*this)[0]);
  return pub_key;
}

PublicKey::PublicKey() : SafeBytes(crypto_box_PUBLICKEYBYTES) {
  
}

SecretSignKey::SecretSignKey() : SafeBytes(crypto_sign_SECRETKEYBYTES) {
  
}


PublicSignKey::PublicSignKey() : SafeBytes(crypto_sign_PUBLICKEYBYTES) {
  
}

Nonce::Nonce() : SafeBytes(crypto_box_NONCEBYTES) {
  reset();
}

void Nonce::reset() {
  Curve::inst().random_bytes(*this);
}


Seed::Seed() : SafeBytes(crypto_box_SEEDBYTES) {}

Seed::Seed(SafeBytes const &bytes) : SafeBytes(bytes) {
  assert(bytes.size() == size());
}


HashKey::HashKey() : SafeBytes(crypto_generichash_KEYBYTES) {}


KeyPair::KeyPair() {
  crypto_box_keypair(&pub[0], &priv[0]);
}


SignKeyPair::SignKeyPair() {
  crypto_sign_keypair(&pub[0], &priv[0]);
}




Hash::Hash(Bytes &m) : hash(crypto_generichash_BYTES) {
  crypto_generichash(&hash[0], hash.size(),
		     &m[0], m.size(), NULL, 0);
}

Hash::Hash(Bytes &m, HashKey &k) : hash(crypto_generichash_BYTES) {
  crypto_generichash(&hash[0], hash.size(),
		     &m[0], m.size(), &k[0], k.size());
}


EncryptedMessage::EncryptedMessage(Bytes &message_, PublicKey &pub, SecretKey &priv) : message(message_), enc_message(message_.size() + crypto_box_MACBYTES) {  
  if (crypto_box_easy(&enc_message[0], &message[0],
		      message.size(), &nonce[0],
		      &pub[0], &priv[0]) != 0)
    throw Err("encryption failed");
}
 
EncryptedMessage::EncryptedMessage(Bytes &enc_message_) : enc_message(enc_message_) {
  
}

bool EncryptedMessage::decrypt(PublicKey &pub, SecretKey &priv, Nonce &nonce) {
  if (enc_message.size() <= crypto_box_MACBYTES)
    return false;
    message.resize(enc_message.size() - crypto_box_MACBYTES);
    
    return crypto_box_open_easy(&message[0], &enc_message[0],
				enc_message.size(), &nonce[0],
				&pub[0], &priv[0]) == 0;
}


SealedMessage::SealedMessage(Bytes &message_, PublicKey &pub) : message(message_), sealed_message(message_.size() + crypto_box_SEALBYTES) {
  crypto_box_seal(&sealed_message[0], &message[0], message.size(), &pub[0]);
}

SealedMessage::SealedMessage(Bytes &sealed_message_) : sealed_message(sealed_message_) {
}
  
bool SealedMessage::decrypt(PublicKey &pub, SecretKey &priv) {
  if (sealed_message.size() <= crypto_box_SEALBYTES)
    return false;
  message.resize(sealed_message.size() - crypto_box_SEALBYTES);
  
  return crypto_box_seal_open(&message[0], &sealed_message[0], sealed_message.size(), &pub[0], &priv[0]) == 0;
}


SignedMessage::SignedMessage(Bytes &message_, SecretSignKey &sk) : message(message_), signed_message(message_.size() + crypto_sign_BYTES) {
  if (crypto_sign(&signed_message[0], 0,
		  &message[0], message.size(), &sk[0]) != 0)
    throw Err("Signing failed");
}

SignedMessage::SignedMessage(Bytes &signed_message_) : signed_message(signed_message_) {}

bool SignedMessage::verify(PublicSignKey &pub) {
  if (signed_message.size() <= crypto_sign_BYTES)
    return false;
  message.resize(signed_message.size() - crypto_sign_BYTES);
  return crypto_sign_open(&message[0], 0,
			  &signed_message[0], signed_message.size(),
			  &pub[0]) == 0;
}

