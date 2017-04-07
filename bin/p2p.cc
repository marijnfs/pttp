#include <zmq.h>
#include <cassert>
#include <vector>
#include <iostream>
#include <cstdint>
#include <thread>

#include <capnp/message.h>
#include <capnp/serialize.h>
#include <capnp/serialize-packed.h>

#include <kj/io.h>

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

struct GTD {

  int operator()() {
    return 0;
  }
};

int main(int argc, char **argv) {
  assert(argc > 1);
  string constr(argv[1]);
  
  Socket sock(ZMQ_ROUTER, constr.c_str());

  set<string> ips;
  
  GTD gtd;

  enum Types {
    SERVE,
    N_TYPES
  };
  
  while (true) {
    auto td = gtd();

    
    if (td == SERVE) {    
      vector<Bytes> msg = sock.recv_multi();
      
      for (auto m : msg)
	cout << m << " ";
      cout << endl;
      cout << "nmsg: " << msg.size() << endl;

      //kj::ArrayInputStream ais(msg[2].kjp());
      //::capnp::InputStreamMessageReader reader(ais);

      ReadMessage message(msg[2]);
      auto reader = message.root<Message>();
      //cout << "port: " << b.getPort() << endl;
      
      auto content = reader.getContent();
      
      //cout << content << endl;
      switch (content.which()) {
      case Message::Content::HELLO: {
	  auto hello = content.getHello();
	  cout << "hello " << hello.getPort() << endl;
	  break;
      }
      default: {
	cout << "default" << endl;
	break;
      }
      }
      
      //auto addrresp = bytes_to_t<AddrResponse>(msg[2]);
      //for (auto addr : addrresp.addresses())
      //  cout << addr << endl;
      sock.send_multi(msg);
    }
  }

  return 0;
}
