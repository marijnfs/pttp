#include <zmq.h>
#include <cassert>
#include <vector>
#include <iostream>
#include <cstdint>
#include <unistd.h>

#include "err.h"
#include "socket.h"

using namespace std;

int main() {
  auto sock = Context::inst().socket(ZMQ_PUB, "tcp://127.0.0.1:1234");

  int n(0);
  while (true) {
    cout << "send request" << endl;
    vector<uint8_t> bytes(1);
    bytes[0] = n;

    sock.send(bytes);
    ++n;
    sleep(1);
  }

  return 0;
}
