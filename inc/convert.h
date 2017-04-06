#ifndef __CONVERT_H__
#define __CONVERT_H__

#include <string>
#include <set>

#include "type.h"
#include "err.h"

#include <capnp/message.h>
#include <capnp/serialize.h>
#include <capnp/serialize-packed.h>

using namespace std;

template <typename T>
inline T bytes_to_t(Bytes &b) {
  T t;
  std::string s(reinterpret_cast<char*>(&b[0]), b.size());
  if (!t.ParseFromString(s))
    throw Err("Could'nt parse from string");
  return t;
};


template <typename T>
inline Bytes t_to_bytes(T &t) {
  std::string buf;
  t.SerializeToString(&buf);
  Bytes bytes(buf.size());
  memcpy(&bytes[0], &buf[0], buf.size());
  return buf;
};


struct ReadMessage {
    ReadMessage(Bytes &bytes) : reader(bytes.kjwp()) {
  
    }

  template <typename T>
  auto root() {
    return reader.getRoot<T>();
  }
  ::capnp::FlatArrayMessageReader reader;
};
  

/*struct ReadMessage {
  //::capnp::MallocAllocator cap_message;

ReadMessage(Bytes &bytes) : cap_message(bytes.size()) {
  
  
}

  template <typename T>
  auto reader() {
    auto readMessageUnchecked<T>(cap_message.);
    return r;
  }
  }*/

struct WriteMessage {
    ::capnp::MallocMessageBuilder cap_message;

  template <typename T>
  auto builder() {
    return cap_message.initRoot<T>();
  }

  Bytes bytes() {
    auto cap_data = messageToFlatArray(cap_message);
    return Bytes(cap_data.begin(), cap_data.size());    
  }
};

template <typename T>
inline T bytes_to_cap(Bytes &b) {
  T t;

  std::string s(reinterpret_cast<char*>(&b[0]), b.size());
  if (!t.ParseFromString(s))
    throw Err("Could'nt parse from string");
  return t;
};


template <typename T>
inline Bytes cap_to_bytes(T &t) {
  std::string buf;
  t.SerializeToString(&buf);
  Bytes bytes(buf.size());
  memcpy(&bytes[0], &buf[0], buf.size());
  return buf;
};



#endif
