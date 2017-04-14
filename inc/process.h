#ifndef __PROCESS_H__
#define __PROCESS_H__

#include <vector>
#include "type.h"
#include "curve.h"
#include "process.h"
#include "messages.capnp.h"

bool create_transaction(::Transaction::Builder &transaction_builder, std::vector<SignKeyPair> &accounts, std::vector<int> &amounts);
Bytes create_transaction(std::vector<SignKeyPair> &accounts, std::vector<int> &amounts);

bool process_transaction(Bytes &b, std::vector<PublicSignKey> *accounts, std::vector<int> *amounts);  //only verifies internal witnesses
bool process_transaction(::Transaction::Reader &b, std::vector<PublicSignKey> *accounts, std::vector<int> *amounts);  //only verifies internal witnesses

#endif
