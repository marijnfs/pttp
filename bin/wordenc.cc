#include "curve.h"

#include <iostream>
#include <fstream>
#include <algorithm>
#include <bitset>

using namespace std;

string msg_str("");

int main(int argc, char **argv) {
  if (argc < 3) {
    cerr << "usage: " << endl;
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

  vector<string> enc_words;
  for (int i(2); i < argc; ++i)
    enc_words.push_back(argv[i]);

    
  //Bytes msg_b(argv[1]);


  bitset<11*24> bits;

  int idx(0);
  for (auto w : enc_words) {
    
    uint n = find(words.begin(), words.end(), w) - words.begin();
    if (n == words.size()) return -1;
    cout << n << endl;
    
    
    for (int i(0); i < 11; ++i) {
      bits[idx++] = (n % 2);
      n >>= 1;
    }
  }

  Bytes msg_b(bits);
  cout << bits.size() << endl;
  cout << bits << endl;
  cout << msg_b.size() << endl;
  cout << msg_b << endl;
  Bytes pass_b(argv[1]);

  Bytes salt_b(8);
  Curve::inst().random_bytes(salt_b);

  //HardHashSalt salt;
  Hash salt_h(salt_b, 16);
  HardHashSalt salt(salt_h);
  //copy(salt_h.begin(), salt_h.end(), salt.begin());
  
  HardestHash h(pass_b, salt);
  
  Hash hh(h, msg_b.size());
  msg_b.x_or(hh);

  //cout << "msg: " << msg_b << endl;
  //cout << "salt: " << salt_b << endl;
  cout << msg_b << " " << salt_b << endl;

  
  
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
