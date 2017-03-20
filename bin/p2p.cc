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

enum MessageType {
  HELLO = 0,
  WELCOME = 1,
  REQ_LIST = 2,
  
};

int main(int argc, char **argv) {
  assert(argc > 1);
  string constr(argv[1]);
  
  Socket sock(ZMQ_ROUTER, constr.c_str());

  vector<Bytes> message;
  Bytes mtype(1);
  mtype[0] = HELLO;
  message.push_back(mtype);
  
  ::capnp::MallocMessageBuilder message_builder;

  auto message_obj = message_builder.initRoot<Hello>();
  message_obj.setPub("asdf");
  message_obj.setPort(3);

  auto flat_array = messageToFlatArray(message_builder);
  auto data = flat_array.asBytes();

  Bytes msg_bytes(reinterpret_cast<uint8_t*>(data.begin()), reinterpret_cast<uint8_t*>(data.end()));
  cout << msg_bytes << endl;
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
