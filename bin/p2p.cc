#include <zmq.h>
#include <cassert>
#include <vector>
#include <iostream>
#include <cstdint>
#include <thread>
#include <map>
#include <ctime>
#include <chrono>
#include <algorithm>

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
#include "gtd.h"

#include "estimate_outside_ip.h"

using namespace std;


struct Connection {
  Socket sock;
  GTD gtd;
  std::string ip;
  
  Connection(std::string addr) : sock(ZMQ_REQ, addr.c_str()), ip(addr) {
    cout << "connecting to :" << addr << endl;
  }
};

std::string my_ip;

std::mutex ip_list_mutex;
set<string> ip_list;

set<Bytes> seen;

void manage_connection(Connection *con) {
  con->gtd.add(Task(HELLO));
  
  while (true) {
    auto task = con->gtd();

    switch (task.type)
      {
      case HELLO: {
	WriteMessage msg;
	auto b = msg.builder<Message>();
	auto h = b.getContent().initHello();
	h.setIp(my_ip);
	auto data = msg.bytes();
	con->sock.send(data);
	auto result = con->sock.recv();
	break;
      }
      case SEND:
      case SENDPRIORITY : {
	con->sock.send(task.data);
	auto result = con->sock.recv();
	break;
      }
      case REQ_IPS:
	{
	  WriteMessage msg;
	  auto b = msg.builder<Message>();
	  b.getContent().initGetPeers();
	  auto data = msg.bytes();
	  con->sock.send(data);
	  auto result = con->sock.recv();
	  
	  ReadMessage in_msg(result);
	  auto msg_reader = in_msg.root<Message>();
	  auto content = msg_reader.getContent();
	  assert(content.which() == Message::Content::IP_LIST);
	  auto new_ip_list = content.getIpList();
	  auto new_ips = new_ip_list.getIps();
	  {
	    std::lock_guard<std::mutex> lock(ip_list_mutex);
	    for (size_t n(0); n < new_ips.size(); ++n) {
	      ip_list.insert(new_ips[n]);
	    cout << new_ips[n].cStr() << endl;
	    }
	    
	  }
	  break;
	}
      }
  }
}

vector<Connection*> connections;
const int N_CONN(8);

void manage() {
  GTD gtd;
  gtd.add(Task{REQ_IPS}, std::chrono::seconds(4));
  gtd.add(Task{EXPAND_CONNECTION}, std::chrono::seconds(1));
  //Socket sock(ZMQ_ROUTER, constr.c_str());
  cout << "n task: " << gtd.q.size() << endl;
  cout << gtd.q.top().time << " " << (gtd.q.top().type == REQ_IPS) << endl;
  vector<std::thread> threads;
  
  while (true) {
    Task task = gtd();
    switch (task.type)
      {
      case REQ_IPS: {
	cout << "req" << endl;
	//inspect current connections, close if needed
	
	for (auto &con : connections) {
	  con->gtd.add(Task{REQ_IPS});
	}
	
	//plan next expand
	gtd.add(task, std::chrono::seconds(10));
	break;
      }

      case EXPAND_CONNECTION: {
	cout << "expand" << endl;
	int n_ips = ip_list.size();
	int desired(min(N_CONN, n_ips));

	int n_con = connections.size();
	int n_new = max(desired - n_con, 0);

	vector<string> ips_tmp(ip_list.begin(), ip_list.end());
	random_shuffle(ips_tmp.begin(), ips_tmp.end());
	cout << "ips: " << ips_tmp << endl;

	//make a set to compare against
	set<string> current_ips;
	for (auto &con : connections)
	  current_ips.insert(con->ip);
	current_ips.insert(my_ip);
	
	int ip_idx(0);
	for (int n(0); n < n_new; ++n) {
	  cout << "selecting new ip" << endl;
	  string new_ip;
	  while (true) {
	    if (ip_idx >= ips_tmp.size())
	      break;
	    new_ip = ips_tmp[ip_idx];
	    if (!current_ips.count(new_ip)) {
	      break;
	    } else {
	      ++ip_idx;
	    }
	  }
	  cout << "found: " << new_ip << endl;
	  if (new_ip.size()) {
	    connections.push_back(new Connection(new_ip));
	    //std::thread t(manage_connection, connections[0]);
	    threads.push_back(std::move(std::thread(manage_connection, last(connections))) );
	  }
	}
	
	//plan next expand
	gtd.add(task, std::chrono::seconds(20));
	break;
      }

     }
  }
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
    if (hard_hash[0] == 0 && hard_hash[1] == 0) {
      cout << n << ": [" << b << "] " << endl;
      cout << hard_hash << endl;
      for (auto con : connections) {
	con->gtd.add(Task{SENDPRIORITY, b, 0});
      }
    }
    n++;
  }
}

void read_lines() {
  while (true) {
    string line;
    getline(cin, line);
    cout << "[" << line << "]" << endl;
    WriteMessage msg;
    auto msg_b = msg.builder<Message>();
    msg_b.getContent().setText(line);
    for (auto con : connections) {
      auto b = msg.bytes();
      con->gtd(Task(SEND, b));
    }
  }
}

void serve() {

}

int main(int argc, char **argv) {
  cout << "my ip: " << estimate_outside_ip() << endl;

  assert(argc > 1);
  string constr(argv[1]);
  my_ip = constr;

  for (int n(2); n < argc; ++n)
    ip_list.insert(argv[n]);
  
  Socket sock(ZMQ_ROUTER, constr.c_str());

  set<string> ips;  
  map<string, int> utxo;

  thread manage_thread(manage);
  //thread mine_thread(mine);
  thread readline_thread(read_lines);

  while (true) {
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
    switch (content.which())
      {
      case Message::Content::HELLO: {
	auto hello = content.getHello();
	cout << "hello " << hello.getIp().cStr() << endl;
	std::lock_guard<std::mutex> lock(ip_list_mutex);
	ip_list.insert(hello.getIp());
	auto wc = content_builder.initWelcome();
      
	break;
      }
      case Message::Content::GET_PEERS: {
	auto iplist_builder = content_builder.initIpList();
	auto iplist = iplist_builder.initIps(ip_list.size());
	int n(0);

	std::lock_guard<std::mutex> lock(ip_list_mutex);
	for (auto ip : ip_list) {
	  iplist.set(n++, const_cast<char*>(ip.c_str()));
	}
	break;
      }
      case Message::Content::BLOCK: {
	HardHash hard_hash(msg[2]);
	if (hard_hash[0] == 0 && hard_hash[1] == 0) {
	  content_builder.initOk();
	} else
	  content_builder.initReject();
	break;
      }
      case Message::Content::TRANSACTION: {
	//process transaction
	vector<PublicSignKey> accounts;
	vector<int> amounts;
	  
	auto tx = content.getTransaction();
	if (process_transaction(tx, &accounts, &amounts)) {
	  content_builder.initOk();
	  cout << amounts << endl;
	} else {
	  content_builder.initReject();
	  cout << "fail" << endl;
	}
	break;
      }
      case Message::Content::TEXT: {
	auto text = content.getText();
	Hash msg_hash(msg[2]);
	if (seen.count(msg_hash) == 0) {
	  cout << "MESSAGE: " << string(text) << endl;
	  seen.insert(msg_hash);
	  for (auto con : connections)
	    con->gtd(Task(SEND, msg[2]));
	}
	content_builder.initOk();
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

  return 0;
}
