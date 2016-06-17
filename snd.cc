#include <zmq.h>
#include <cassert>
#include <vector>
#include <iostream>

using namespace std;

int main() {
  void *ctx = zmq_ctx_new();
  if (!ctx)
    return 1;
  
  void *sock = zmq_socket(ctx, ZMQ_REQ);
  if (!sock)
    return 1;
  
  if (zmq_connect(sock, "tcp://*:5555") == -1)
    return 1;

  {
    zmq_msg_t *msg = new zmq_msg_t;
    int rc = zmq_msg_init_size (msg, 10);
    vector<char> bytes(10);
    for (size_t i(0); i < bytes.size(); ++i)
      bytes[i] = i;
    copy(&bytes[0], &bytes[bytes.size()], (char*)zmq_msg_data(msg));
    assert (rc == 0);
    
    rc = zmq_msg_send(msg, sock, 0);//ZMQ_DONTWAIT for polling
    assert (rc == 0);
    //int nbytes = zmq_recv (socket, &msg, 0);
  }

  {
      zmq_msg_t *msg = new zmq_msg_t;
      
      int rc = zmq_msg_init (msg);
      
      rc = zmq_msg_recv(msg, sock, 0);//ZMQ_DONTWAIT for polling
      assert (rc >= 0);
      
      vector<char> bytes(rc);
      copy((char*)zmq_msg_data(msg), (char*)zmq_msg_data(msg) + rc, &bytes[0]);
      for (size_t i(0); i < bytes.size(); ++i)
	cout << bytes[i] << " ";
      cout << endl;
  }


  
  
  if (ctx)
    zmq_ctx_shutdown(ctx);
  
  return 0;
}
