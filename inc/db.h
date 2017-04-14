#ifndef __DB_H__
#define __DB_H__

#include "rocksdb/db.h"
#include "rocksdb/slice.h"
#include "rocksdb/options.h"

#include <string>
#include <cassert>

struct DB {
  DB(std::string path_) : path(path_) {
    options.IncreaseParallelism();
    options.OptimizeLevelStyleCompaction();
    // create the DB if it's not already present
    options.create_if_missing = true;

    rocksdb::Status s = rocksdb::DB::Open(options, path, &db);
    assert(s.ok());
  }

  bool put(Bytes &id, Bytes &data) {
    rocksdb::Status s = db->Put(wo, rocksdb::Slice(id.ptr<char*>(), id.size()), rocksdb::Slice(data.ptr<char*>(), data.size()));
    return s.ok();
  }

  bool del(Bytes &id) {
    rocksdb::Status s = db->Delete(wo, rocksdb::Slice(id.ptr<char*>(), id.size()));
    return s.ok();
  }
  
  bool get(Bytes &id, Bytes *data) {
    std::string value;
    rocksdb::Status s = db->Get(ro, rocksdb::Slice(id.ptr<char*>(), id.size()), &value);
    
    if (s.ok()) *data = Bytes(value);
    return s.ok();
  }

  rocksdb::DB *db;
  rocksdb::Options options;
  rocksdb::WriteOptions wo;
  rocksdb::ReadOptions ro;

  std::string path;
};

#endif
