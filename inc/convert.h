#ifndef __CONVERT_H__
#define __CONVERT_H__

#include <string>
#include <set>

#include "type.h"
#include "err.h"
#include "segwit_addr.h"

#include <capnp/message.h>
#include <capnp/serialize.h>
#include <capnp/serialize-packed.h>

#include <string>

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

inline std::string bytes_to_bech32(std::string pre, Bytes &b) {
  //cout << int(b[0]) << endl;
  if (b.size() == 0) return "";
  Bytes enc5;

  size_t l(0), r(5);
  size_t n(b[0]);
  size_t last(0);
  while (true) {
    size_t idl(l / 8), idr(r / 8);
    if (idl >= b.size()) break;
    if (idr != last && idr < b.size()) {
      n += int(b[idr]) << (5 - r % 8);
      last = idr;
    }
    int val = n % (1 << 5);
    enc5.push_back(val);
    //cout << val << endl;
    
    n >>= 5;
    l += 5; r += 5;
  }


  int sl(5);
  std::string s(pre.size() + enc5.size() + 8, ' ');
  bech32_encode(&s[0], &pre[0], &enc5[0], enc5.size());
  
  return s;
}



#endif
