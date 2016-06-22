#ifndef __SOCKET_H__
#define __SOCKET_H__

#include <string>
#include <vector>
#include <cstdint>
#include <zmq.h>
#include <cassert>
#include <iostream>

#include "type.h"
#include "err.h"

struct Socket {

Socket(void *ctx, int type_, std::string addr_) : type(type_), addr(addr_) {
  sock = zmq_socket(ctx, type);
  if (!sock)
    throw Err("Couldnt create socket");

  if (type == ZMQ_REP || type == ZMQ_PUB) {
    std::cout << "binding to " << addr << std::endl;
    if (zmq_bind(sock, addr.c_str()) == -1)
      throw Err("couldnt connect");
  } else {
    std::cout << "connecting to " << addr << std::endl;
    if (zmq_connect(sock, addr.c_str()) == -1)
      throw Err("couldnt connect");
  }
}

  void send(Bytes &data) {
    zmq_msg_t *msg = new zmq_msg_t;
    int rc = zmq_msg_init_size (msg, data.size());
    assert(rc != -1);
    std::copy(&data[0], &data[data.size()], (char*)zmq_msg_data(msg));

    rc = zmq_msg_send(msg, sock, 0);//ZMQ_DONTWAIT for polling
    assert (rc != -1);
  }

  Bytes recv() {
    zmq_msg_t *msg = new zmq_msg_t;
    int rc = zmq_msg_init (msg);
    if (rc == -1)
      throw Err("msg init failed");
    
    rc = zmq_msg_recv(msg, sock, 0);//ZMQ_DONTWAIT for polling
    if (rc == -1)
      throw Err("recieving failed failed");

    Bytes data(rc);
    char *data_ptr = (char*)zmq_msg_data(msg);
    std::copy(data_ptr, data_ptr + rc, &data[0]);
    return data;
  }

  void subscribe(std::string pref) {
    assert(type = ZMQ_SUB);
    zmq_setsockopt (sock, ZMQ_SUBSCRIBE, pref.c_str(), pref.size());
  }
  
  std::string addr;
  int type;
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
