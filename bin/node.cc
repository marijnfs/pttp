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
#include "util.h"
#include "messages.capnp.h"
#include "process.h"

using namespace std;



int main(int argc, char **argv) {
  assert(argc > 1);
  string constr(argv[1]);

  // *** create transaction
  vector<SignKeyPair> accounts(3);
  vector<int> amounts(3);
  amounts[0] = 10;
  amounts[1] = -4;
  amounts[2] = 12;

  cout << "priv: " << bytes_to_bech32("pv", accounts[0].priv) << endl;
  Bytes tx = create_transaction(accounts, amounts);
  cout << "sdf" << endl;
  
  Hash hash(tx);
  cout << "hash: " << hash << endl;
  HardHash hardhash(tx);
  cout << "hardhash: " << hardhash << endl;

  SignKeyPair test_pair;
  cout << test_pair.pub << endl;
  cout << "pub: " << bytes_to_bech32("pb", test_pair.pub) << endl;
  cout << "priv: " << bytes_to_bech32("pv", test_pair.priv) << endl;
  Bytes pwd("apasswd");
  HardestHash pwd_hash(pwd);
  cout << "pwd: " << pwd_hash << endl;
  SecretKey enc_key;
  copy(pwd_hash.begin(), pwd_hash.end(), enc_key.begin());
  
  vector<PublicSignKey> new_accounts;
  vector<int> new_amounts;
  cout << process_transaction(tx, &new_accounts, &new_amounts) << endl;
  cout << new_amounts.size() << endl;
  cout << new_amounts << endl;
  cout << new_accounts << endl;

  cout << bytes_to_bech32("pc", new_accounts[0]) << endl;
  
  // **** Send message
  Socket sock(ZMQ_REQ, constr.c_str());
  //WriteMessage hello_message;
  //xauto hello_builder = hello_message.builder<Hello>();

  {
    WriteMessage message;
    //auto builder = message.builder<Message>();
    
    auto builder = message.builder<Message>();
    auto hello_builder = builder.getContent().initHello();
    hello_builder.setIp("sdfasdf");
    hello_builder.setPort(120);
    
    Bytes data = message.bytes();
    sock.send(data);
    Bytes result = sock.recv();
  }

  {
    WriteMessage message;
    auto builder = message.builder<Message>();
    auto tx = builder.getContent().initTransaction();
    create_transaction(tx, accounts, amounts);
    Bytes b = message.bytes();
    sock.send(b);
    auto result = sock.recv();
  }

  {
    WriteMessage message;
    auto builder = message.builder<Message>();
    builder.getContent().initGetPeers();
    Bytes b = message.bytes();
    sock.send(b);
    auto result = sock.recv();
    ReadMessage read_message(result);
    auto reader = read_message.root<Message>();
    auto list = reader.getContent().getIpList();
    auto ips = list.getIps();
    cout << ips.size() << " ip: " << ips[0].cStr() << endl;
  }
  
  return 0;
}
