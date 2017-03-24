#include <zmq.h>
#include <cassert>
#include <vector>
#include <iostream>
#include <cstdint>
#include <thread>

#include <capnp/message.h>
#include <capnp/serialize.h>
#include <capnp/serialize-packed.h>

#include "err.h"
#include "socket.h"
#include "curve.h"

#include "convert.h"

#include "messages.capnp.h"

using namespace std;



int main(int argc, char **argv) {
  assert(argc > 1);
  string constr(argv[1]);
  
  Socket sock(ZMQ_ROUTER, constr.c_str());


  vector<SignKeyPair> accounts(3);
  
  
  /*  Bytes message(10);
  Curve::inst().random_bytes(message);
  SignedMessage sign_message(message, kp.priv);

  for (int i(0); i < 50000; ++i) {
    sign_message.verify(kp.pub);
    //cout << sign_message.verify(kp.pub) << endl;
    }*/
  
  
  ::capnp::MallocMessageBuilder cap_message;

  ::capnp::MallocMessageBuilder credit_message;
  auto builder = cap_message.initRoot<Transaction>();

  auto credit_set_builder = credit_message.initRoot<CreditSet>();
  auto credit_builder = credit_set_builder.initCredits(3);
  credit_builder[0].setAccount(static_cast<string>(accounts[0].pub));
  credit_builder[0].setAmount(-10);

  credit_builder[1].setAccount(static_cast<string>(accounts[1].pub));
  credit_builder[1].setAmount(-14);

  credit_builder[2].setAccount(static_cast<string>(accounts[2].pub));
  credit_builder[2].setAmount(24);

  auto credit_data = messageToFlatArray(credit_message).asBytes();
  Bytes test(credit_data.begin(), credit_data.size());

  //encrypt/authenticate and decrypt test

  KeyPair kp1, kp2;
  EncryptedMessage enc_message(test, kp2.pub, kp1.priv);
  cout << enc_message.message << endl;
  cout << enc_message.enc_message << endl;
  cout << enc_message.nonce << endl;

  Hash hash(enc_message.enc_message);
  cout << hash.hash << endl;
  
  EncryptedMessage dec_message(enc_message.enc_message);
  cout << dec_message.decrypt(kp1.pub, kp2.priv, enc_message.nonce) << endl;
  cout << dec_message.message << endl;
  
  
  //sign and verify test
  SignedMessage credit_signed(test, accounts[0].priv);
  cout << credit_signed.message << endl;
  cout << credit_signed.signed_message << endl;
  
  SignedMessage tmp(credit_signed.signed_message);
  cout << tmp.verify(accounts[0].pub) << endl;
 
  auto sig_builder = builder.initSignatures(3);

  auto data = messageToFlatArray(cap_message).asBytes();
  //auto data = flat_array.asBytes();
  for (auto d : data)
    cout << d;
  cout << endl;
      /*while (true) {
      vector<Bytes> msg = sock.recv_multi();

      for (auto m : msg)
      cout << m << " ";
      cout << endl;
      //auto addrresp = bytes_to_t<AddrResponse>(msg[2]);
      //for (auto addr : addrresp.addresses())
      //  cout << addr << endl;
      sock.send_multi(msg);
      }*/

  return 0;
}
