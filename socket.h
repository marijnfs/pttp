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
  Socket(void *ctx, int type_, std::string addr_);

  void send(Bytes &data);
  
  Bytes recv();

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

  Socket socket(int type, std::string addr);

  void *ctx;
  static Context *s;
};


#endif
