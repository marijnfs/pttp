#include "socket.h"

using namespace std;

Context *Context::s = 0;


Socket::Socket(void *ctx, int type_, string addr_) : type(type_), addr(addr_) {
  sock = zmq_socket(ctx, type);
  if (!sock)
    throw Err("Couldnt create socket");

  if (type == ZMQ_REP || type == ZMQ_PUB) {
    cout << "binding to " << addr << endl;
    if (zmq_bind(sock, addr.c_str()) == -1)
      throw Err("couldnt connect");
  } else {
    cout << "connecting to " << addr << endl;
    if (zmq_connect(sock, addr.c_str()) == -1)
      throw Err("couldnt connect");
  }
}

void Socket::send(Bytes &data) {
  zmq_msg_t *msg = new zmq_msg_t;
  int rc = zmq_msg_init_size (msg, data.size());
  assert(rc != -1);
  copy(&data[0], &data[data.size()], (char*)zmq_msg_data(msg));

  rc = zmq_msg_send(msg, sock, 0);//ZMQ_DONTWAIT for polling
  assert (rc != -1);
}

Bytes Socket::recv() {
  zmq_msg_t *msg = new zmq_msg_t;
  int rc = zmq_msg_init (msg);
  if (rc == -1)
    throw Err("msg init failed");
    
  rc = zmq_msg_recv(msg, sock, 0);//ZMQ_DONTWAIT for polling
  if (rc == -1)
    throw Err("recieving failed failed");

  Bytes data(rc);
  char *data_ptr = (char*)zmq_msg_data(msg);
  copy(data_ptr, data_ptr + rc, &data[0]);
  return data;
}

void Socket::subscribe(string pref) {
  assert(type = ZMQ_SUB);
  zmq_setsockopt (sock, ZMQ_SUBSCRIBE, pref.c_str(), pref.size());
}

Context::Context() : ctx(0) {
  init();
}
  
Context &Context::inst() {
  if (!Context::s) {
    Context::s = new Context();
  }
      
  return *Context::s;
}

void Context::init() {
  ctx = zmq_ctx_new();
  if (!ctx)
    throw Err("Couldnt create context");
}

Context::~Context() {
  if (ctx)
    zmq_ctx_shutdown(ctx);
}

Socket Context::socket(int type, string addr) {
  return Socket(ctx, type, addr);
}
