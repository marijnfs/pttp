#include "curve.h"

#include <iostream>

using namespace std;

string msg_str("");


int main(int argc, char **argv) {
  if (argc != 4) {
    cerr << "usage: " << endl;
    return 1;
  }

  string msg_hex(argv[1]);
  string salt_hex(argv[2]);
  Bytes pass_b(argv[3]);
  
  Bytes salt_b;
  salt_b.from_hex(salt_hex);

  //HardHashSalt salt;
  Hash salt_h(salt_b, 16);
  HardHashSalt salt(salt_h);
  //copy(salt_h.begin(), salt_h.end(), salt.begin());
  
  HardestHash h(pass_b, salt);
  
  Bytes msg_b;
  msg_b.from_hex(msg_hex);

  Hash hh(h, 32);
  KeyPair pair(hh);

  
  SealedMessage msg(msg_b);
  msg.decrypt(pair.pub, pair.priv);
  
  cout << "msg: " << string(msg.message) << endl;

  
  
  /*KeyPair pair(h);
 
  SealedMessage msg(s, pair.pub);

  cout << msg.sealed_message << endl;

    
  KeyPair pair2(h);

  ostringstream oss;
  oss << msg.sealed_message;
  cout << oss.str() << endl;
  Bytes msg_h;
  msg_h.from_hex(oss.str());
  SealedMessage msg2(msg_h);
  cout << msg2.decrypt(pair2.pub, pair2.priv) << endl;
  cout << crypto_secretbox_NONCEBYTES << " " << crypto_secretbox_KEYBYTES << " " << crypto_secretbox_MACBYTES << " " << crypto_auth_BYTES << endl;*/
  return 0;
}
