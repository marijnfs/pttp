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


  SignKeyPair kp;
  Bytes message(10);
  Curve::inst().random_bytes(message);
  SignedMessage sign_message(message, kp.priv);

  for (int i(0); i < 50000; ++i) {
    sign_message.verify(kp.pub);
    //cout << sign_message.verify(kp.pub) << endl;
  }
  
  
  ::capnp::MallocMessageBuilder cap_message;

  auto builder = cap_message.initRoot<TransactionSet>();
  auto list_builder = builder.initTransactions(3);
  list_builder[0].setTransactionId("sdf");
  auto witness_builder = list_builder[0].initSignatures(1);
  witness_builder[0].setData("df");
  auto blabuilder = list_builder[0].initBla(3);
  blabuilder[0] = "sdf";
  //list_builder/
    
  auto flat_array = messageToFlatArray(cap_message);
  auto data = flat_array.asBytes();
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
