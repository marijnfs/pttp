#include <zmq.h>
#include <cassert>
#include <vector>
#include <iostream>
#include <cstdint>
#include <thread>

#include "err.h"
#include "socket.h"
#include "curve.h"

#include "msg.pb.h"
#include "convert.h"

using namespace std;


int main(int argc, char **argv) {
  assert(argc > 1);
  string constr(argv[1]);
  
  Socket sock(ZMQ_ROUTER, constr.c_str());

  while (true) {
    vector<Bytes> msg = sock.recv_multi();

    auto addrresp = bytes_to_t<AddrResponse>(msg[2]);
    for (auto addr : addrresp.addresses())
      cout << addr << endl;
    //for (auto part : msg)
    //  cout << part << " ";
    //cout << endl;

    //msg[2] = "bla";
    //auto msg = sock.recv();

    sock.send_multi(msg);
  }

  Context::shutdown();
  return 0;
}
