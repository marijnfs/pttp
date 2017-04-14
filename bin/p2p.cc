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

#include "util.h"
#include "convert.h"
#include "process.h"
#include "messages.capnp.h"
#include "db.h"

using namespace std;


struct GTD {

  int operator()() {
    return 0;
  }
};

struct Node {
  Bytes pub;
  
};

Bytes get_current_transaction_hash() {
  
}

void mine() {
  int n(0);
  while (true) {
    WriteMessage msg;
    auto builder = msg.builder<Block>();
    
    Bytes rnd(32);
    Curve::inst().random_bytes(rnd);
    Hash hash(rnd);
    
    builder.setPrevHash(hash.kjp());
    builder.setTransactionHash(hash.kjp());
    builder.setUtxoHash(hash.kjp());
    builder.setTime(124);
    Curve::inst().random_bytes(rnd);
    
    builder.setNonce(rnd.kjp());
    auto b = msg.bytes();
    HardHash hard_hash(b);
    if (hard_hash[0] == 0) {
      cout << n << ": [" << b << "] " << endl;
      cout << hard_hash << endl;
    }
    n++;
  }
}

int main(int argc, char **argv) {
  //mine();
  assert(argc > 1);
  string constr(argv[1]);
  
  Socket sock(ZMQ_ROUTER, constr.c_str());

  set<string> ips;
  
  GTD gtd;

  enum Types {
    SERVE,
    N_TYPES
  };


  set<string> ip_list;
  ip_list.insert("asdf");
  ip_list.insert("asdfasdf");
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
      case Message::Content::GET_PEERS: {
	auto iplist_builder = content_builder.initIpList();
	auto iplist = iplist_builder.initIps(ip_list.size());
	int n(0);
	vector<Bytes> ip_bytes;
	for (auto ip : ip_list) {
	  ip_bytes.push_back(ip);
	  cout << ip << endl;
	  iplist.set(n++, const_cast<char*>(ip.c_str()));
	}

	break;
      }
	
      case Message::Content::TRANSACTION: {
	//process transaction
	  vector<PublicSignKey> accounts;
	  vector<int> amounts;
	  
	  auto tx = content.getTransaction();
	  if (!process_transaction(tx, &accounts, &amounts))
	    cout << "fail" << endl;
	  cout << amounts << endl;

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
