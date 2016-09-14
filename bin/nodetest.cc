#include <zmq.h>
#include <cassert>
#include <vector>
#include <iostream>
#include <cstdint>
#include <unistd.h>

#include "err.h"
#include "socket.h"
#include "curve.h"

#include "msg.pb.h"
#include "convert.h"

using namespace std;

int main(int argc, char **argv) {
  assert(argc > 1);
  string constr(argv[1]);
  
  
  Socket sock(ZMQ_REQ, constr.c_str());

  AddrResponse resp;
  resp.add_addresses("test");
  resp.add_addresses("test2");
  resp.add_addresses("test3");

  Bytes message(t_to_bytes(resp));
  
  sock.send(message);
  
  cout << "rec" << endl;
  auto msg = sock.recv();
  cout << "got: " << msg << endl;
  //cout << "sending 2" << endl;
  //sock.send(message);
  //sock.recv();
  
  
  Context::shutdown();
  return 0;
}
