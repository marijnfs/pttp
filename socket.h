#ifndef __SOCKET_H__
#define __SOCKET_H__

#include <string>
#include <vector>
#include <cstdint>
#include <zmq.h>
#include <cassert>

#include "err.h"

typedef std::vector<uint8_t> DataVec;

struct Socket {

Socket(void *ctx, int type, std::string addr_) : addr(addr_) {
  sock = zmq_socket(ctx, type);
  if (!sock)
    throw Err("Couldnt create socket");

  if (type == ZMQ_REP) {
    if (zmq_bind(sock, addr.c_str()) == -1)
      throw Err("couldnt connect");
  } else {
    if (zmq_connect(sock, addr.c_str()) == -1)
      throw Err("couldnt connect");
  }
}

  void send(DataVec &data) {
    zmq_msg_t *msg = new zmq_msg_t;
    int rc = zmq_msg_init_size (msg, data.size());
    assert(rc != -1);
    std::copy(&data[0], &data[data.size()], (char*)zmq_msg_data(msg));

    rc = zmq_msg_send(msg, sock, 0);//ZMQ_DONTWAIT for polling
    assert (rc != -1);
  }

  DataVec recv() {
    zmq_msg_t *msg = new zmq_msg_t;
    int rc = zmq_msg_init (msg);
    if (rc == -1)
      throw Err("msg init failed");
    
    rc = zmq_msg_recv(msg, sock, 0);//ZMQ_DONTWAIT for polling
    if (rc == -1)
      throw Err("recieving failed failed");

    DataVec data(rc);
    char *data_ptr = (char*)zmq_msg_data(msg);
    std::copy(data_ptr, data_ptr + rc, &data[0]);
    return data;
  }

  std::string addr;
  void *sock;
};

struct Context {
  void *ctx;
  static Context *s;

Context() : ctx(0) {
    init();
  }
  
  static Context &inst() {
    if (!Context::s) {
      Context::s = new Context();
    }
      
    return *Context::s;
  }

  void init() {
    ctx = zmq_ctx_new();
    if (!ctx)
      throw Err("Couldnt create context");
  }

  ~Context() {
    if (ctx)
      zmq_ctx_shutdown(ctx);
  }

  Socket socket(int type, std::string addr) {
    return Socket(ctx, type, addr);
  }
};


#endif
