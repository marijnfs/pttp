#include <zmq.h>
#include <cassert>
#include <vector>
#include <iostream>
#include <cstdint>
#include <thread>

#include "err.h"
#include "socket.h"
#include "curve.h"

using namespace std;


int main(int argc, char **argv) {
  assert(argc > 1);
  string constr(argv[1]);
  
  auto sock = Context::inst().socket(ZMQ_ROUTER, constr.c_str());

  while (true) {
    cout << "receiving" << endl;
    vector<Bytes> msg = sock.recv_multi();

    for (auto part : msg)
      cout << part << " ";
    cout << endl;

    //msg[2] = "bla";
    //auto msg = sock.recv();


    sock.send_multi(msg);
        sock.send_multi(msg);
  }
   
  return 0;
}
