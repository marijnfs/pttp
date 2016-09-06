#include "socket.h"
#include "util.h"

using namespace std;

Context *Context::s = 0;


Socket::Socket(void *ctx, int type_, string addr_) : type(type_), addr(addr_) {
  sock = zmq_socket(ctx, type);
  if (!sock)
    throw Err("Couldnt create socket");

  if (type == ZMQ_REP || type == ZMQ_PUB || type == ZMQ_ROUTER || type == ZMQ_DEALER) {
    cout << "binding to " << addr << endl;
    if (zmq_bind(sock, addr.c_str()) == -1)
      throw Err("couldnt connect");
  } else {
    cout << "connecting to " << addr << endl;
    if (zmq_connect(sock, addr.c_str()) == -1)
      throw Err("couldnt connect");
  }
}

Socket::~Socket() {
  zmq_close(sock);
}

void Socket::send(Bytes &data) {
  zmq_msg_t *msg = new zmq_msg_t;
  int rc = zmq_msg_init_size (msg, data.size());
  assert(rc != -1);
  memcpy((char*)zmq_msg_data(msg), &data[0], data.size());
  //copy(&data[0], &data[data.size()], (char*)zmq_msg_data(msg));
  cout << "sending " << data.size() << endl;
  rc = zmq_msg_send(msg, sock, 0); //ZMQ_DONTWAIT for polling
  assert (rc != -1);
}

void Socket::send_multi(vector<Bytes> &data) {
  for (int i(0); i < data.size(); ++i) {
    if (data[i].size() == 0) {
      int rc = zmq_send(sock, 0, 0, (i != data.size() - 1 ? ZMQ_MORE : 0));
      assert (rc != -1);
      continue;
    }
    cout << "sending part " << i << " " << data[i] << endl;
    zmq_msg_t *msg = new zmq_msg_t;
    int rc = zmq_msg_init_size (msg, data[i].size());
    assert(rc != -1);
    //if (data[i].size()) copy(&(data[i][0]), &(data[i][data.size()]), (char*)zmq_msg_data(msg));
    memcpy((char*)zmq_msg_data(msg), &data[0], data.size());
    cout << (i != data.size() - 1 ? ZMQ_MORE : 0) << endl;
    rc = zmq_msg_send(msg, sock, (i != data.size() - 1 ? ZMQ_MORE : 0));
    assert (rc != -1);
  }
}

Bytes Socket::recv() {
  zmq_msg_t *msg = new zmq_msg_t;
  int rc = zmq_msg_init (msg);
  if (rc == -1)
    throw Err("msg init failed");
    
  rc = zmq_msg_recv(msg, sock, 0);//ZMQ_DONTWAIT for polling
  if (rc == -1)
    throw Err("recieving failed failed");

  //if (rc > MAX_MSG)
  // throw Err("Message too big");
  
  Bytes data(rc);
  memcpy(&data[0], (char*)zmq_msg_data(msg), rc);

  zmq_msg_close(msg);
  return data;
}

vector<Bytes> Socket::recv_multi() {
  vector<Bytes> msgs;

  int more(0);
  size_t moresz(sizeof(more));
  
  while (true) {
    zmq_msg_t *msg = new zmq_msg_t;
    int rc = zmq_msg_init (msg);
    if (rc == -1)
      throw Err("msg init failed");

    cout << "recving" << endl;
    rc = zmq_msg_recv(msg, sock, 0);//ZMQ_DONTWAIT for polling
    if (rc == -1)
      throw Err("recieving failed failed");
    cout << "got something: " << endl;
    msgs.push_back(Bytes(rc));
    
    Bytes &data = last(msgs);
    memcpy(&data[0], (char*)zmq_msg_data(msg), data.size());
    cout << data << endl;
    int more(0);
    size_t moresz(sizeof(more));
    
    rc = zmq_getsockopt(sock, ZMQ_RCVMORE, &more, &moresz);
    cout << rc << " " << zmq_strerror(zmq_errno()) << endl;
    if (rc == -1) throw Err("Sockop failed");

    zmq_msg_close(msg);
    if (!more)
      break;
  }
  return msgs;
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
