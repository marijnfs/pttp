#ifndef __DB_H__
#define __DB_H__

#include "rocksdb/db.h"
#include "rocksdb/slice.h"
#include "rocksdb/options.h"

#include <string>

struct DB {
  DB() {}
  
  
  rocksdb::DB *db;
  rocksdb::Options options;
  rocksdb::WriteOptions wo;
  rocksdb::ReadOptions ro;
};

#endif
