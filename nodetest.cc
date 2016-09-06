#include <zmq.h>
#include <cassert>
#include <vector>
#include <iostream>
#include <cstdint>
#include <unistd.h>

#include "err.h"
#include "socket.h"
#include "curve.h"

//#include "msg.pb.h"

using namespace std;

int main(int argc, char **argv) {
  assert(argc > 1);
  string constr(argv[1]);
  
  SafeBytes message(20);
  for (int i(0); i < message.size(); ++i)
    message[i] = i;
  
  
  auto sock = Context::inst().socket(ZMQ_REQ, constr.c_str());

  
  sock.send(message);
  //sock.send(message);
  cout << "rec" << endl;
  sock.recv_multi();
  cout << "sending 2" << endl;
  sock.send(message);
  sock.recv();

  
    
  return 0;
}
