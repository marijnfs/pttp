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

bool create_transaction(vector<SignKeyPair> &accounts, vector<int> amounts) {
  int n = accounts.size();
  if (amounts.size() != n) return false;

  WriteMessage credit_message;
  auto credit_set_builder = credit_message.builder<CreditSet>();
  auto credit_builder = credit_set_builder.initCredits(n);

  for (int i(0); i < n; ++i) {
    //credit_builder[i].setAccount((kj::ArrayPtr<kj::byte>)accounts[i].pub);
    credit_builder[i].setAccount(accounts[i].pub.kjp());
    credit_builder[i].setAmount(amounts[i]);
  }
  
  auto credit_data = credit_message.bytes();
  
  ::capnp::MallocMessageBuilder transaction_message;
  auto transaction_builder = transaction_message.initRoot<Transaction>();

  transaction_builder.initCreditSets(1)[0] = credit_data.kjp();
  

  //bla[0].set(bla2);
  
}



int main(int argc, char **argv) {
  assert(argc > 1);
  string constr(argv[1]);
  
  Socket sock(ZMQ_REQ, constr.c_str());
  //WriteMessage hello_message;
  //xauto hello_builder = hello_message.builder<Hello>();

  WriteMessage message;
  //auto builder = message.builder<Message>();
  auto builder = message.builder<Message>();
  auto hello_builder = builder.getContent().initHello();
  hello_builder.setPort(120);
  
  Bytes data = message.bytes();
  sock.send(data);
  Bytes result = sock.recv();

  

  //vector<SignKeyPair> accounts(3);
  
  
  /*  Bytes message(10);
  Curve::inst().random_bytes(message);
  SignedMessage sign_message(message, kp.priv);

  for (int i(0); i < 50000; ++i) {
    sign_message.verify(kp.pub);
    //cout << sign_message.verify(kp.pub) << endl;
    }*/

  
  /*::capnp::MallocMessageBuilder cap_message;

  auto builder = cap_message.initRoot<Transaction>();

  ::capnp::MallocMessageBuilder credit_message;
  auto credit_set_builder = credit_message.initRoot<CreditSet>();
  auto credit_builder = credit_set_builder.initCredits(3);
  credit_builder[0].setAccount(accounts[0].pub.kjp());
  credit_builder[0].setAmount(-10);

  credit_builder[1].setAccount(accounts[1].pub.kjp());
  credit_builder[1].setAmount(-14);

  credit_builder[2].setAccount(accounts[2].pub.kjp());
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
  cout << "hash: " << hash.hash << endl;
  
  EncryptedMessage dec_message(enc_message.enc_message);
  cout << dec_message.decrypt(kp1.pub, kp2.priv, enc_message.nonce) << endl;
  cout << dec_message.message << endl;
  
  
  //sign and verify test
  SignedMessage credit_signed(test, accounts[0].priv);
  cout << credit_signed.message << endl;
  cout << credit_signed.signed_message << endl;
  
  SignedMessage tmp(credit_signed.signed_message);
  cout << tmp.verify(accounts[0].pub) << endl;
  cout << "encrypt priv:" << kp1.priv << endl;
  cout << kp1.pub << endl;
  cout << "sign priv:" << accounts[0].priv << endl;
  cout << accounts[0].pub << endl;
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
