#ifndef __ERR_H__
#define __ERR_H__

#include <string>
#include <exception>
#include <sstream>

#include <zmq.h>



struct Err : std::exception{
Err(std::string msg_) : msg(msg_){}
  
  virtual const char *what() const throw() {return &msg[0];}
  virtual ~Err() throw() {}
  std::string msg;
};

struct ZMQErr : public Err {
 ZMQErr(std::string msg_) : Err(msg_) {}
  virtual const char *what() const throw() {
    std::ostringstream oss;
    oss << msg << " : " << zmq_strerror(zmq_errno());
    return oss.str().c_str();
  }
  virtual ~ZMQErr() throw() {}
  
  
};

inline void assertthrow(bool check, Err err) {
  if (!check)
    throw err;
}

#endif
