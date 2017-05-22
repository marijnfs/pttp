#include <zmq.h>
#include <cassert>
#include <vector>
#include <iostream>
#include <cstdint>
#include <thread>
#include <map>
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
  vector<Bytes> current_set;
  uint64_t start_time;
  
  SetReconsiler(Bytes hash_) : hash(hash_), N(0), start_time(now_ms()) {}
  
  void filter(std::vector<Bytes*> set) {
    current_set.clear();

    cout << "N: " << N << endl;
    cout << "n filters: " << filters.size() << endl;
    cout << "latest set:" << endl;

    for (auto &s : set) {
      bool inset(true);
      for (auto &f : filters) {
	if (!f.has(const_cast<Bytes&>(*s))) {
	  inset = false;
	  break;
	}
      }
      if (inset) {
	current_set.push_back(*s);
	cout << (*s) << endl;
      }
    }
  }

  bool check() {
    if (current_set.size() == 0) {
      cout << "empty" << endl;
      return false;
    }

    vector<Bytes*> pointer_set(current_set.size());
    for (int n(0); n < pointer_set.size(); ++n)
      pointer_set[n] = &current_set[n];
    sort(pointer_set.begin(), pointer_set.end(), [](Bytes *l, Bytes *r){return *l < *r;});
    auto current_hash = hash_vec(pointer_set);
    cout << "hashes: " << current_hash << " " << hash << endl;
    return hash == current_hash;
  }

  
};



void mine() {
  int n(0);

  while (true) {
    WriteMessage msg;
    auto builder = msg.builder<Block>();

    //todo: get right hashes
    //builder.setPrevHash(hash.kjp());
    //builder.setTransactionHash(hash.kjp());
    //builder.setUtxoHash(hash.kjp());

    //cout << ms_uint << endl;
    builder.setTime(now_ms());
    
    Bytes rnd(32);
    random_bytes(rnd);
    builder.setNonce(rnd.kjp());

    auto b = msg.bytes();
    HardHash hard_hash(b);
    if (hard_hash[0] == 0) {// && hard_hash[1] == 0) {
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


namespace pttp {
  struct Account {
    Bytes pub;
    int64_t amount;
    uint64_t signature_count;
    Bytes commitment;
  };
    
  struct Credit {
    Bytes pub;
    int64_t amount;
  };
  
  struct Transaction {
    vector<Credit> credits;
    vector<Bytes> witnesses;

    Transaction(Bytes b) {
      ReadMessage r(b);
      auto tx = r.root<::Transaction>();
      auto credit_set = tx.getCreditSet();
      Bytes credit_data(credit_set.begin(), credit_set.end());

      Hash credit_hash(credit_data);
            
      ReadMessage credit_msg(credit_data);
      auto credit_reader = credit_msg.root<CreditSet>();
      auto cap_credits = credit_reader.getCredits();
      auto cap_witnesses = tx.getSignatures();


      //check signatures
      int n_neg(0);
      int n = cap_credits.size();
     
      credits.resize(n);
      witnesses.resize(n);
      for (int i(0); i < n; ++i) {
	int amount = cap_credits[i].getAmount();
	credits[i].amount = amount;
	auto pub = cap_credits[i].getAccount();
	Bytes pub_bytes(pub.begin(), pub.end());
	credits[i].pub = pub_bytes;
	PublicSignKey pub_key(pub_bytes);
	
	if (amount < 0) {
	  if (n_neg >= witnesses.size())
	    throw "";
	  
	  auto witness_data = cap_witnesses[n_neg].getData();
	  Bytes witness(witness_data.begin(), witness_data.end());
	  witnesses[n_neg] = witness;
	  Signature sig(witness);
	  if (!sig.verify(credit_data, pub_key))
	    throw "";
	  n_neg++;
	}
      }
      
    }
    
    Bytes credit_bytes() {
      WriteMessage credit_message;
      auto credit_set_builder = credit_message.builder<CreditSet>();
      auto credit_set = credit_set_builder.initCredits(credits.size());
      
      for (int i(0); i < credits.size(); ++i) {
	//credit_builder[i].setAccount((kj::ArrayPtr<kj::byte>)accounts[i].pub);
	credit_set[i].setAccount(credits[i].pub.kjp());
	credit_set[i].setAmount(credits[i].amount);
      }
      
      auto credit_data = credit_message.bytes();
      return credit_data;
    }

    Bytes bytes() {
      WriteMessage msg;
      auto tx_b = msg.builder<::Transaction>();
      auto sig_b = tx_b.initSignatures(witnesses.size());
      auto cred_data = credit_bytes();
      for (int n(0); n < witnesses.size(); ++n) {
	sig_b[n].setType(0);
	sig_b[n].setData(witnesses[n].kjp());
      }
      tx_b.setCreditSet(cred_data.kjp());
      return msg.bytes();
    }
    
  };
  
  
  struct BlockHeader {
    Bytes hash;
    Bytes prev_hash, tx_hash, utxo_hash;
    Bytes nonce;
    uint64_t T;
    uint64_t height;

    BlockHeader() : T(0), height(0) {}
    
    void from_bytes(Bytes &b) {
      ReadMessage msg(b);
      auto block_r = msg.root<Block>();
      
      hash = block_r.getPrevHash();
      tx_hash = block_r.getTransactionHash();
      utxo_hash = block_r.getUtxoHash();
      nonce = block_r.getNonce();
      T = block_r.getTime();
    }
    
    Bytes bytes() {
      WriteMessage msg;
      auto builder = msg.builder<Block>();
      
      builder.setPrevHash(prev_hash.kjp());
      builder.setTransactionHash(tx_hash.kjp());
      builder.setUtxoHash(utxo_hash.kjp());
      builder.setTime(T);
      
      builder.setNonce(nonce.kjp());
      return msg.bytes();
    }
    
    Bytes calculate_hash() {
      Bytes b = bytes();
      return HardHash(b);
    }
  };
  
  struct BlockChain {
    map<Bytes, BlockHeader*> blocks;
    map<Bytes, int> heights;
    
    //vector<BlockHeader*> new_blocks;
    Bytes latest_block_hash;

    
    map<Bytes, Transaction*> txs;  //all txs
    map<Bytes, Account*> utxos;
    
    set<Bytes> mempool; //hashes
    vector<Bytes> latest_set; //hashes (valid subset of mempool against current utxos)
    Bytes latest_hash;

    Bytes zero_block;
    
    map<Bytes, vector<Bytes>> hash_sets;
    map<Bytes, vector<Bytes>> utxo_sets; //indexed by pub key
    map<Bytes, int64_t> balance; //essentially utxo set in searchable form indexed by pub key
    
    std::mutex chain_mutex;
    
    BlockChain() : zero_block(32)  {}
    
    vector<Bytes*> get_tx_hashes(Bytes hash) {
    }

    vector<Bytes*> get_utxo_hashes(Bytes hash) {
    }

    vector<Bytes*> get_tx_hashes(Bytes start_block, Bytes end_block) {
    }

    vector<Bytes*> get_utxo_hashes(Bytes start_block, Bytes end_block) {
    }

    vector<Bytes*> get_block_hashes(Bytes start_block = Bytes(), Bytes end_block = Bytes()) {
    }
    
    void add(Transaction tx) {
      auto b = tx.bytes();
      Hash hash(b);
      if (!txs.count(hash)) {
	txs[hash] = new Transaction(tx);
	mempool.insert(hash);
      }
    }
    
    Bytes get_latest() {
      map<Bytes, int64_t> balance_cpy(balance);
      vector<Bytes*> hash_ptrs;
      latest_set.clear();
      
      std::lock_guard<std::mutex> lock(chain_mutex);
      
      for (auto &hash : mempool) {
	bool valid(true);
	auto tx = txs[hash];
	set<Bytes> pub_set; //transactions in credit set should be unique
	for (auto &cred : tx->credits) {
	  if (pub_set.count(cred.pub)) {valid = false; break;}
	  pub_set.insert(cred.pub);
	  if (cred.amount < 0 && (!balance_cpy.count(cred.pub) || balance_cpy[cred.pub] < -cred.amount)) {valid = false; break;}
	}
	if (valid) {
	  latest_set.push_back(hash);
	  hash_ptrs.push_back(const_cast<Bytes*>(&hash));
	  for (auto &cred : tx->credits) {
	    if (balance_cpy.count(cred.pub))
	      balance_cpy[cred.pub] += cred.amount;
	    else
	      balance_cpy[cred.pub] = cred.amount;
	  }
	}
      }
      Bytes hash = hash_vec(hash_ptrs);
      latest_hash = hash;
      return hash;
    }
    
    bool add_block_if_ok(BlockHeader &header) {
      Bytes hash = header.calculate_hash();
      if (hash[0] != 0) return false;

      if (hash_sets.count(header.tx_hash)) { //do we have the transactions?
	
      }
    }

    void mark_blocks() {
      for (auto b : blocks) heights[b.first] = 0;
      
      for (auto b : blocks) {
	  if (heights[b.first] == -1)
	    break;
	vector<Bytes> list;
	list.push_back(b.first);
	auto cur = b.second;
	while (true) {

	  if (cur->prev_hash == zero_block) {
	    int h = 0;
	    for (auto it = list.rbegin(); it != list.rend(); ++it) { //set heights
	      heights[*it] = h + 1;
	      ++h;
	    }
	    break;
	  }
	  if (!blocks.count(cur->prev_hash)) { //set neg height
	    for (auto e : list) heights[e] = -1;
	    break;
	  }
	  if (heights[cur->prev_hash] == -1) {
	    for (auto e : list) heights[e] = -1;
	    break;
	  }
	  if (heights[cur->prev_hash] > 0) {
	    int h = heights[cur->prev_hash];
	    for (auto it = list.rbegin(); it != list.rend(); ++it) { //set proper heights
	      heights[*it] = h + 1;
	      ++h;
	    }
	    break;
	  }
	  list.push_back(cur->prev_hash);
	  cur = blocks[cur->prev_hash];
	}
      }
    }
  };

  struct BlockChainReconsiler {
    BlockChain *chain;

    BlockChainReconsiler(BlockChain *chain_) : chain(chain_) {}
    BlockChainReconsiler() : chain(0) {}
    
    vector<BlockHeader> new_headers;
    map<Bytes, SetReconsiler*> hash_reconsilers;

    void add_filter(Bytes hash, int n, Bloom bloom) {
      if (!hash_reconsilers.count(hash))
	hash_reconsilers[hash] = new SetReconsiler(hash);
    }

    bool check(Bytes hash) {
      if (!hash_reconsilers.count(hash)) return false;
      hash_reconsilers[hash]->check();
    }

    void init_hash(Bytes hash) {
      if (!hash_reconsilers.count(hash))
	hash_reconsilers[hash] = new SetReconsiler(hash);
    }

    vector<Bloom> &filters(Bytes hash) {
      hash_reconsilers[hash]->filters;
    }

    bool get_set_or_filter(Bytes hash) {
      if (filters(hash).size() == 0)
	return false;

      //reconsiler.apply_filters(hash);
      SetReconsiler &r(*hash_reconsilers[hash]);
      return r.current_set.size() < r.N;
    }
    
    void apply_filters() {
      
    }
  };
}

pttp::BlockChain block_chain;
pttp::BlockChainReconsiler reconsiler;

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

	  //Send filter request
	  WriteMessage msg;
	  auto b = msg.builder<Message>();
	  auto reqhashset_builder = b.getContent().initReqSetFilter();
	  reqhashset_builder.getReq().setTxSetHash(req_hash.kjp());

	  auto data = msg.bytes();
	  con->sock.send(data);
	  auto result = con->sock.recv();

	  //read filter
	  ReadMessage in_msg(result);
	  auto filter = in_msg.root<SetFilter>();
	  int n = filter.getN();
	  auto bloom_read = filter.getBloom();
	  
	  auto p = bloom_read.getP();
	  cout << "p: " << p  << endl;
	  if (p == 0) //no filter
	    break;

	  auto key = bloom_read.getHashKey();
	  Bytes key_bytes(key.begin(), key.end());
	  auto bloom_data = bloom_read.getData();
	  Bytes bloom_bytes(bloom_data.begin(), bloom_data.end());
	  
	  std::lock_guard<std::mutex> lock(set_mutex);
	  reconsiler.add_filter(req_hash, n, Bloom(bloom_bytes, int(p), key_bytes));
	  break;
	}
      case REQ_HASHSET:
	{
	  cout << "req hash filter: " << endl;
	  Bytes req_hash = task.data;
	  
	  WriteMessage msg;
	  auto b = msg.builder<Message>();
	  auto reqhashset_builder = b.getContent().initReqSet();
	  
	  reqhashset_builder.getReq().setTxSetHash(req_hash.kjp());
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
	  auto msg_reader = in_msg.root<::Set>();
	  	  
	  auto hashset = msg_reader.getData();
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
	  reconsiler.init_hash(hash);
	  if (reconsiler.check(hash)) {
	    cout << "DONE" << endl;
	    break;
	  }
	  if (reconsiler.get_set_or_filter(hash)) {
	    for (auto &con : connections)
	      con->gtd.add(Task(REQ_HASHSET, hash));
	  } else {
	    for (auto &con : connections)
	      con->gtd.add(Task(REQ_HASHSET_FILTER, hash));
	  }
	  
	  
	  manager_gtd.add(task, std::chrono::seconds(1));
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


void serve(string con_str) {
  Socket sock(ZMQ_ROUTER, con_str.c_str());

  while (true) {
    vector<Bytes> msg = sock.recv_multi();

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
      case Message::Content::REQ_SET_FILTER: {
	cout << "got req hashset filter" << endl;
	auto filter_b = content_builder.initSetFilter();
	Bloom bloom(4999);

	std::lock_guard<std::mutex> lock(set_mutex);

	//filter stuff
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
      case Message::Content::REQ_SET: {
	auto req_reader = content.getReqSet();
	cout << "got req hashset" << endl;
	auto bloom_read = req_reader.getBloom();
	auto bloom_data = bloom_read.getData();
	Bytes bloom_bytes(bloom_data.begin(), bloom_data.end());
	  
	auto p = bloom_read.getP(); //0 if empty
	auto key = bloom_read.getHashKey();
	Bytes key_bytes(key.begin(), key.end());
	Bloom bloom(bloom_bytes, p, key_bytes);
	
	auto req_type = req_reader.getReq();
	switch (req_type.which())
	  {
	  case ::ReqSetFilter::Req::TX_SET_HASH: {
	    std::lock_guard<std::mutex> lock(set_mutex);
	    auto cap_hash = req_type.getTxSetHash();
	    Bytes target_hash(cap_hash);
	    
	    if (block_chain.hash_sets.count(target_hash) == 0) {
	      content_builder.initReject();
	      break;
	    }

	    auto set = block_chain.hash_sets[target_hash];
	    vector<Bytes> bs;
	    for (auto &s : set)
	      if (!bloom.has(const_cast<Bytes&>(s)))
		bs.push_back(block_chain.txs[s]->bytes());//s.kjp()
	    
	    
	    auto hash_set = content_builder.initSet().initData(bs.size());
	    for (int i(0); i < bs.size(); ++i)
	      hash_set.set(i, bs[i].kjp());
	  }
	}
	     
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


int main(int argc, char **argv) {
  reconsiler.chain = &block_chain;
  
  cout << "my ip: " << estimate_outside_ip() << endl;

  //random filler
  for (int i(0); i < 3000; ++i) {
    Bytes b(10);
    Curve::inst().random_bytes(b);
    full_set.insert(b);
  }

  cout << "fullset: " << endl;
  for (auto s : full_set)
    cout << s << endl;
  
    
  assert(argc > 1);
  string con_str(argv[1]);
  my_ip = con_str;

  for (int n(2); n < argc; ++n)
    ip_list.insert(argv[n]);

  thread manage_thread(manage);
  thread mine_thread(mine);
  thread readline_thread(read_lines);
  thread serve_thread(serve, con_str);

  serve_thread.join();
  manage_thread.join();
  readline_thread.join();
  
  return 0;
}
