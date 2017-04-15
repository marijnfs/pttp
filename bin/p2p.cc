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
#include "bloom_filter.h"

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

vector<Connection*> connections;
const int N_CONN(8);

std::string my_ip;


set<string> ip_list;
std::mutex ip_list_mutex;

set<Bytes> seen;

set<Bytes> full_set;
std::mutex set_mutex;

struct SetReconsiler {
  Bytes hash;
  size_t N;
  vector<Bloom> filters;
  vector<Bytes*> current_set;

  SetReconsiler() : N(0) {}
  
  void filter(std::set<Bytes> &set) {
    current_set.clear();

    cout << "N: " << N << endl;
    cout << "n filters: " << filters.size() << endl;
    cout << "latest set:" << endl;
    for (auto &s : set) {
      bool inset(true);
      for (auto &f : filters) {
	if (!f.has(const_cast<Bytes&>(s))) {
	  inset = false;
	  break;
	}
      }
      if (inset) {
	current_set.push_back(const_cast<Bytes*>(&s));
	cout << s << endl;
      }
    }
  }

  bool check_hash() {
    if (current_set.size() == 0) {
      cout << "empty" << endl;
      return false;
    }
    
    sort(current_set.begin(), current_set.end(), [](Bytes *l, Bytes *r){return *l < *r;});
    auto current_hash = hash_vec(current_set);
    cout << "hashes: " << current_set << " " << hash << endl;
    return hash == current_hash;
  }
};

SetReconsiler reconsiler;

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

GTD manager_gtd;

void read_lines() {
  while (true) {
    string line;
    getline(cin, line);
    cout << "[" << line << "]" << endl;
    if (line.find("go") != string::npos)
      manager_gtd.add(Task(SYNC_HASHSET));
      
    WriteMessage msg;
    auto msg_b = msg.builder<Message>();
    msg_b.getContent().setText(line);
    for (auto con : connections) {
      auto b = msg.bytes();
      con->gtd(Task(SEND, b));
    }
  }
}


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
      case REQ_HASHSET_FILTER:
	{
	  cout << "req hash filter: " << endl;
	  Bytes req_hash = task.data;
	  
	  WriteMessage msg;
	  auto b = msg.builder<Message>();
	  auto reqhashset_builder = b.getContent().initReqHashsetFilter();
	  reqhashset_builder.setHash(req_hash.kjp());

	  auto data = msg.bytes();
	  con->sock.send(data);
	  auto result = con->sock.recv();
	  
	  ReadMessage in_msg(result);
	  auto msg_reader = in_msg.root<Message>();
	  auto content = msg_reader.getContent();
	  assert(content.which() == Message::Content::HASHSET_FILTER);
	  auto filter = content.getHashsetFilter();
	  int n = filter.getN();
	  auto bloom_read = filter.getBloom();
	  auto bloom_data = bloom_read.getData();
	  Bytes bloom_bytes(bloom_data.begin(), bloom_data.end());
	  
	  auto p = bloom_read.getP();
	  cout << "p: " << p  << endl;
	  if (p == 0) //no filter
	    break;
	  auto key = bloom_read.getHashKey();
	  Bytes key_bytes(key.begin(), key.end());
	  
	  std::lock_guard<std::mutex> lock(set_mutex);
	  reconsiler.N = n;
	  reconsiler.filters.push_back(Bloom(bloom_bytes, int(p), key_bytes));

	  break;
	}
      case REQ_HASHSET:
	{
	  cout << "req hash filter: " << endl;
	  Bytes req_hash = task.data;
	  
	  WriteMessage msg;
	  auto b = msg.builder<Message>();
	  auto reqhashset_builder = b.getContent().initReqHashset();
	  reqhashset_builder.setHash(req_hash.kjp());
	  auto bloom_builder = reqhashset_builder.initBloom();

	  //make filter of what we have
	  Bloom bloom(4999);
	  for (auto h : full_set)
	    bloom.set(h);
	  Bytes bloom_bytes = bloom.bytes();
	  bloom_builder.setP(bloom.P);
	  bloom_builder.setHashKey(bloom.key.kjp());
	  bloom_builder.setData(bloom_bytes.kjp());

	  auto data = msg.bytes();
	  con->sock.send(data);
	  auto result = con->sock.recv();
	  
	  ReadMessage in_msg(result);
	  auto msg_reader = in_msg.root<Message>();
	  auto content = msg_reader.getContent();
	  assert(content.which() == Message::Content::HASHSET);
	  auto hashset = content.getHashset().getSet();
	  cout << "got set: " << hashset.size() << endl;
	  std::lock_guard<std::mutex> lock(set_mutex);
	  for (auto h : hashset) {
	    Bytes b(h.begin(), h.end());
	    full_set.insert(b);
	  }
	  break;
	}
      }
  }
}


void manage() {
  manager_gtd.add(Task{REQ_IPS}, std::chrono::seconds(4));
  manager_gtd.add(Task{EXPAND_CONNECTION}, std::chrono::seconds(1));
  //Socket sock(ZMQ_ROUTER, constr.c_str());
  vector<std::thread> threads;
  
  while (true) {
    Task task = manager_gtd();
    switch (task.type)
      {
      case SYNC_HASHSET:
	{
	  std::lock_guard<std::mutex> lock(set_mutex);
	  Bytes hash = task.data;
	  reconsiler.hash = hash;
	  if (reconsiler.check_hash()) {
	    cout << "DONE" << endl;
	    break;
	  }
	  if (reconsiler.filters.size() == 0)
	    for (auto &con : connections)
	      con->gtd.add(Task(REQ_HASHSET_FILTER, reconsiler.hash));
	  else {
	    reconsiler.filter(full_set);
	    
	    if (reconsiler.current_set.size() < reconsiler.N) {
	      for (auto &con : connections)
		con->gtd.add(Task(REQ_HASHSET, reconsiler.hash));
	    } else {
	      for (auto &con : connections)
		con->gtd.add(Task(REQ_HASHSET_FILTER, reconsiler.hash));
	    }
	  }
	  manager_gtd.add(task, std::chrono::seconds(3));
	  break;
	}

      case REQ_IPS: {
	cout << "req" << endl;
	//inspect current connections, close if needed
	
	for (auto &con : connections) {
	  con->gtd.add(Task{REQ_IPS});
	}
	
	//plan next expand
	manager_gtd.add(task, std::chrono::seconds(10));
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
	manager_gtd.add(task, std::chrono::seconds(20));
	break;
      }

     }
  }
}
  


void serve() {

}

int main(int argc, char **argv) {
  cout << "my ip: " << estimate_outside_ip() << endl;

  for (int i(0); i < 10; ++i) {
    Bytes b(10);
    Curve::inst().random_bytes(b);
    full_set.insert(b);
  }

  cout << "fullset: " << endl;
  for (auto s : full_set)
    cout << s << endl;
  
    
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
      
    /*for (auto m : msg)
      cout << m << " ";
      cout << endl;*/
    
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
      case Message::Content::REQ_HASHSET_FILTER: {
	cout << "got req hashset filter" << endl;
	auto filter_b = content_builder.initHashsetFilter();
	Bloom bloom(4999);

	std::lock_guard<std::mutex> lock(set_mutex);
	for (auto s : full_set)
	  bloom.set(s);
	filter_b.setN(full_set.size());
	auto bloom_builder = filter_b.initBloom();
	Bytes bloom_bytes = bloom.bytes(); ///Shady, gets destroyed before message is sent
	bloom_builder.setP(bloom.P);
	bloom_builder.setHashKey(bloom.key.kjp());
	bloom_builder.setData(bloom_bytes.kjp());
	
	break;
      }
      case Message::Content::REQ_HASHSET: {
	cout << "got req hashset" << endl;
	auto req_reader = content.getReqHashset();
	auto hash = req_reader.getHash();
	auto bloom_read = req_reader.getBloom();
	auto bloom_data = bloom_read.getData();
	Bytes bloom_bytes(bloom_data.begin(), bloom_data.end());
	  
	auto p = bloom_read.getP();
	if (p == 0) //no filter
	  break;
	auto key = bloom_read.getHashKey();
	Bytes key_bytes(key.begin(), key.end());
	Bloom bloom(bloom_bytes, p, key_bytes);
	
	std::lock_guard<std::mutex> lock(set_mutex);
	
	vector<Bytes*> bs;
	for (auto &s : full_set)
	  if (!bloom.has(const_cast<Bytes&>(s)))
	    bs.push_back(const_cast<Bytes*>(&s));//s.kjp()
	
	auto hash_set = content_builder.initHashset().initSet(bs.size());
	for (int i(0); i < bs.size(); ++i)
	  hash_set.set(i, bs[i]->kjp());
	     
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
