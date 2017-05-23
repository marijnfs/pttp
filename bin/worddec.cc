#include "curve.h"

#include <iostream>
#include <fstream>

using namespace std;

string msg_str("");


int main(int argc, char **argv) {
  if (argc != 4) {
    cerr << "usage: msg salt pass" << endl;
    return 1;
  }

  ifstream words_file("../words/english.txt");
  vector<string> words;
  while (words_file) {
    string w;
    words_file >> w;
    if (w.size())
      words.push_back(w);
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
  
  Hash hh(h, msg_b.size());
  msg_b.x_or(hh);

  //cout << "msg: " << msg_b << endl;
  cout << msg_b << endl;
  auto bitset = msg_b.bitset<11*24>();
  cout << bitset << endl;
  int idx(0);
  for (int i(0); i < 24; ++i) {
    int m(0);
    for (int n(0); n < 11; ++n) {
      m |= (bitset[idx++] << n);
    }
      
    cout << words[m] << endl;
  }
  
  
  
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
