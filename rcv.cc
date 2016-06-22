#include <zmq.h>
#include <cassert>
#include <vector>
#include <iostream>
#include <cstdint>

#include "err.h"
#include "socket.h"
#include "curve.h"

using namespace std;


int main() {
  auto sock = Context::inst().socket(ZMQ_SUB, "tcp://127.0.0.1:1234");
  sock.subscribe("");
  while (true) {
    vector<uint8_t> bytes = sock.recv();
    for (size_t i(0); i < bytes.size(); ++i)
      cout << (int)bytes[i] << " ";
    cout << endl;
  }
   
  return 0;
}
