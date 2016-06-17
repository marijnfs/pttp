#ifndef __ERR_H__
#define __ERR_H__

#include <string>
#include <exception>

struct Err : std::exception{
Err(std::string msg_) : msg(msg_){}
  
  virtual const char *what() const throw() {return &msg[0];}
  virtual ~Err() throw() {}
  std::string msg;
};


#endif
