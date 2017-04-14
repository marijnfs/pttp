#ifndef __GTD_H__
#define __GTD_H__


#include <queue>
#include <thread>
#include <mutex>
#include <ctime>
#include <chrono>


enum TaskType {
  SENDPRIORITY = 0,
  SEND = 1,
  SYNC_UTXO  = 2,
  EXPAND_CONNECTION = 3,
  REQ_IPS
};

struct Task {
  TaskType type;
  Bytes data;
  std::time_t time;
};

bool operator<(Task const&l, Task const &r) {
  if (l.time == r.time)
    return l.type < r.type;
  return l.time < r.time;
}

struct GTD {
  std::priority_queue<Task> q;
  std::mutex m;
  
  Task operator()() {
    while (true) {
      {
	std::lock_guard<std::mutex> lock(m);
	if (q.size() != 0) {
	  auto task = q.top();
	  if (task.time < std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())) {
	    q.pop();
	    return task;
	  }
	}
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }

  void add(Task task) {
    std::lock_guard<std::mutex> lock(m);
    q.push(task);
  }

  void add(Task task, std::chrono::milliseconds dur) {
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    std::time_t future = std::chrono::system_clock::to_time_t(now + dur);
    task.time = future;
    
    std::lock_guard<std::mutex> lock(m);
    q.push(task);
  }
};


#endif
