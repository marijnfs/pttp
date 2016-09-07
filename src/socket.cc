#include "socket.h"
#include "util.h"

using namespace std;

Context *Context::s = 0;


Socket::Socket(int type_, string addr_) : type(type_), addr(addr_) {
  sock = zmq_socket(Context::inst().ctx, type);
  assertthrow(sock, ZMQErr("Couldnt create socket"));

  if (type == ZMQ_REP || type == ZMQ_PUB || type == ZMQ_ROUTER || type == ZMQ_DEALER) {
    cout << "binding to " << addr << endl;
    assertthrow(zmq_bind(sock, addr.c_str()) != -1, ZMQErr("couldnt connect"));
  } else {
    if (type == ZMQ_REQ) {
      //assertthrow( zmq_setsockopt(sock, ZMQ_IDENTITY, "123", 3) != -1, ZMQErr("Couldnt set identity") );
      //int one(1);
      //assertthrow( zmq_setsockopt(sock, ZMQ_IMMEDIATE, &one, sizeof(one)) != -1, ZMQErr("Could set immediate"));
    }

    cout << "connecting to " << addr << endl;
    assertthrow(zmq_connect(sock, addr.c_str()) != -1, ZMQErr("couldnt connect"));
  }
}

Socket::~Socket() {
  cout << "Socket destructor called" << endl;
  zmq_close(sock);
}

void Socket::send(Bytes &data, bool more) {
  zmq_msg_t *msg = new zmq_msg_t;
  int rc = zmq_msg_init_size (msg, data.size());
  assertthrow(rc != -1, ZMQErr("Failed msg init"));
  memcpy((char*)zmq_msg_data(msg), &data[0], data.size());
  //copy(&data[0], &data[data.size()], (char*)zmq_msg_data(msg));
  cout << "sending [" << data << "]" << (more ? "+" : "") << endl;
  rc = zmq_msg_send(msg, sock, more ? ZMQ_MORE : 0); //ZMQ_DONTWAIT for polling
  assertthrow(rc != -1, ZMQErr("Failed sending"));
}

void Socket::send_multi(vector<Bytes> &msgs) {
  for (int i(0); i < msgs.size(); ++i) {
    cout << "part " << i << ":" << endl;
    send(msgs[i], i != msgs.size() - 1);
  }
}

Bytes Socket::recv() {
  zmq_msg_t *msg = new zmq_msg_t;
  int rc = zmq_msg_init (msg);
  assertthrow(rc != -1, ZMQErr("msg init failed"));
    
  rc = zmq_msg_recv(msg, sock, 0);//ZMQ_DONTWAIT for polling
  assertthrow(rc != -1, ZMQErr("recieving failed failed"));

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
    assertthrow(rc != -1, ZMQErr("msg init failed"));

    cout << "receiving" << endl;
    rc = zmq_msg_recv(msg, sock, 0);//ZMQ_DONTWAIT for polling
    assertthrow(rc != -1, ZMQErr("recieving failed failed"));
    
    msgs.push_back(Bytes(rc));
    
    Bytes &data = last(msgs);
    memcpy(&data[0], (char*)zmq_msg_data(msg), data.size());

    int more(0);
    size_t moresz(sizeof(more));
    rc = zmq_getsockopt(sock, ZMQ_RCVMORE, &more, &moresz);
    assertthrow(rc != -1, ZMQErr("Sockop failed"));

    zmq_msg_close(msg);
    cout << "got: [" << data << "]" << (more ? "+" : "" ) << endl;
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
  assertthrow(ctx, ZMQErr("Couldnt create context"));
}

void Context::shutdown() {
  if (Context::s) {
    Context::s->~Context();
  }
}

Context::~Context() {
  if (ctx)
    zmq_ctx_shutdown(ctx);
  cout << "Context Destroyed" << endl;
}

//Socket Context::socket(int type, string addr) {
//  return Socket(ctx, type, addr);
//}
