#include "process.h"
#include "convert.h"
#include "messages.capnp.h"
#include "util.h"

using namespace std;

//functions for processing data

bool create_transaction(::Transaction::Builder &transaction_builder, vector<SignKeyPair> &accounts, vector<int> &amounts) {
  int n = accounts.size();
  if (amounts.size() != n) return false;

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
  transaction_builder.setHash(hash.kjp());
  
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
  
  return true;
}

Bytes create_transaction(vector<SignKeyPair> &accounts, vector<int> &amounts) {
  WriteMessage transaction_message;
  auto transaction_builder = transaction_message.builder<Transaction>();
  
  if (!create_transaction(transaction_builder, accounts, amounts))
    throw "fail";
  return transaction_message.bytes();
}

bool process_transaction(Bytes &b, vector<PublicSignKey> *accounts, vector<int> *amounts) {  //only verifies internal witnesses
  ReadMessage msg(b);
  auto tx = msg.root<Transaction>();
  return process_transaction(tx, accounts, amounts);
}

bool process_transaction(::Transaction::Reader &tx, std::vector<PublicSignKey> *accounts, std::vector<int> *amounts) {  //only verifies internal witnesses
  auto credit_set = tx.getCreditSet();
  Bytes credit_data(credit_set.begin(), credit_set.end());

  Hash credit_hash(credit_data);
  auto hash = tx.getHash();
  if (Bytes(hash.begin(), hash.end()) != credit_hash)
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

