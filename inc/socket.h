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
  Socket(int type_, std::string addr_);
  ~Socket();
  void send(Bytes &data, bool more = false);

  Bytes recv();

  void send_multi(std::vector<Bytes> &data);

  std::vector<Bytes> recv_multi();

  
  void subscribe(std::string pref);
  
  std::string addr;
  int type;
  void *sock;
};

struct Context {
  Context();
  
  static Context &inst();

  void init();

  ~Context();

  //Socket socket(int type, std::string addr);

  void *ctx;
  static Context *s;
};


#endif
