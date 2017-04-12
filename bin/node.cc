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

using namespace std;

Bytes create_transaction(vector<SignKeyPair> &accounts, vector<int> amounts) {
  int n = accounts.size();
  if (amounts.size() != n) return false;

  WriteMessage transaction_message;
  auto transaction_builder = transaction_message.builder<Transaction>();

  //setup credits
  WriteMessage credit_message;
  auto credit_set_builder = credit_message.builder<CreditSet>();
  auto credits = credit_set_builder.initCredits(n);

  int n_negative(0);
  for (int i(0); i < n; ++i) {
    //credit_builder[i].setAccount((kj::ArrayPtr<kj::byte>)accounts[i].pub);
    credits[i].setAccount(accounts[i].pub.kjp());
    credits[i].setAmount(amounts[i]);
    if (amounts[i] < 0) n_negative++;
  }

  auto credit_data = credit_message.bytes();
  transaction_builder.setCreditSet(credit_data.kjp());

  //calculate hash
  Hash hash(credit_data);
  transaction_builder.setHash(hash.hash.kjp());
  
  //setup witnesses
  auto witness_builder = transaction_builder.initSignatures(n_negative);
  int c(0);
  vector<Signature> sigs(n_negative);
  for (int i(0); i < n; ++i)
    if (amounts[i] < 0) {
      sigs[c] = Signature(credit_data, accounts[i].priv);
      witness_builder[c].setData(sigs[c].sig.kjp());
      ++c;
    }
    
  //bla[0].set(bla2);
  return transaction_message.bytes();
}

bool process_transaction(Bytes &b, vector<PublicSignKey> *accounts, vector<int> *amounts) {  //only verifies internal witnesses
  ReadMessage msg(b);
  auto tx = msg.root<Transaction>();
  auto credit_set = tx.getCreditSet();
  Bytes credit_data(credit_set.begin(), credit_set.end());

  Hash credit_hash(credit_data);
  auto hash = tx.getHash();
  if (Bytes(hash.begin(), hash.end()) != credit_hash.hash)
    return false;
  
  ReadMessage credit_msg(credit_data);
  auto credit_reader = credit_msg.root<CreditSet>();
  auto credits = credit_reader.getCredits();
  auto witnesses = tx.getSignatures();
  
  int n_neg(0);
  int n = credits.size();
  amounts->resize(n);
  for (int i(0); i < n; ++i) {
    int a = credits[i].getAmount();
    (*amounts)[i] = a;
    auto pub = credits[i].getAccount();
    Bytes pub_bytes(pub.begin(), pub.end());
    PublicSignKey pub_key(pub_bytes);
    accounts->push_back(pub_bytes);
    
    if (a < 0) {
      if (n_neg >= witnesses.size())
	return false;

      auto witness_data = witnesses[n_neg].getData();
      Bytes witness(witness_data.begin(), witness_data.end());
      Signature sig(witness);
      if (!sig.verify(credit_data, pub_key))
	return false;
      n_neg++;
    }
  }

  return true;
}



int main(int argc, char **argv) {
  assert(argc > 1);
  string constr(argv[1]);

  // *** create transaction
  vector<SignKeyPair> accounts(3);
  vector<int> amounts(3);
  amounts[0] = 10;
  amounts[1] = -4;
  amounts[2] = 12;

  Bytes bla(accounts[0].priv);
  cout << bla << endl;
  string test = bytes_to_bech32(bla);
  cout << test << endl;
  cout << "priv: " << bytes_to_bech32(bla) << endl;
  Bytes tx = create_transaction(accounts, amounts);


  
  vector<PublicSignKey> new_accounts;
  vector<int> new_amounts;
  cout << process_transaction(tx, &new_accounts, &new_amounts) << endl;
  cout << new_amounts.size() << endl;
  cout << new_amounts << endl;
  cout << new_accounts << endl;

  cout << bytes_to_bech32(new_accounts[0]) << endl;
  
  // **** Send message
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

  

  
  return 0;
}
