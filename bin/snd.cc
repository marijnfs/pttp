#include <zmq.h>
#include <cassert>
#include <vector>
#include <iostream>
#include <cstdint>
#include <unistd.h>

#include "err.h"
#include "socket.h"
#include "curve.h"

using namespace std;

int main() {
  SafeBytes message(20);
  for (int i(0); i < message.size(); ++i)
    message[i] = i;

  KeyPair pair1, pair2;
  cout << pair1.pub << " " << pair1.priv << endl;
  cout << pair2.pub << " " << pair2.priv << endl;
  cout << message << endl;
  
  EncryptedMessage enc(message, pair1.pub, pair2.priv);
  cout << "enc_message: " << enc.enc_message << endl;

  EncryptedMessage dec(enc.enc_message);
  cout << "success: " <<  dec.decrypt(pair2.pub, pair1.priv, enc.nonce) << endl;
  cout << dec.message << endl;

  
  SealedMessage sealed(message, pair1.pub);
  SealedMessage unsealed(sealed.sealed_message);
  cout << "success: " << unsealed.decrypt(pair1.pub, pair1.priv) << endl;
  cout << unsealed.message << endl;

  SignKeyPair kp3;
  SignedMessage sign(message, kp3.priv);
  cout << sign.signed_message << endl;
  SignedMessage verified(sign.signed_message);

  cout << "signed: " << verified.verify(kp3.pub) << endl;
  cout << verified.message << endl;

  auto sock = Context::inst().socket(ZMQ_PUB, "tcp://127.0.0.1:1234");

  int n(0);
  while (true) {
    cout << "send request" << endl;
    vector<uint8_t> bytes(1);
    bytes[0] = n;

    sock.send(bytes);
    ++n;
    sleep(1);
  }

  return 0;
}
