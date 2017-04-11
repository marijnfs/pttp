#include <zmq.h>
#include <cassert>
#include <vector>
#include <iostream>
#include <cstdint>
#include <thread>
#include <map>

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


struct GTD {

  int operator()() {
    return 0;
  }
};

struct Node {
  Bytes pub;
  
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


  
  map<string, int> utxo;
  
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

      WriteMessage response_msg;
      auto resp_builder = response_msg.builder<Message>();
      auto content_builder = resp_builder.getContent(); //build message from here
      
      //cout << content << endl;
      switch (content.which()) {
      case Message::Content::HELLO: {
	  auto hello = content.getHello();
	  cout << "hello " << hello.getPort() << endl;
	  auto wc = content_builder.initWelcome();
	  
	  break;
      }
      case Message::Content::TRANSACTION: {
	//process transaction
	auto tx = content.getTransaction();
	auto data = tx.getCreditSet();

	break;
      }
      default: {
	cout << "default" << endl;
	break;
      }
      }


      msg[2] = response_msg.bytes();
      //auto addrresp = bytes_to_t<AddrResponse>(msg[2]);
      //for (auto addr : addrresp.addresses())
      //  cout << addr << endl;
      sock.send_multi(msg);
    }
  }

  return 0;
}
