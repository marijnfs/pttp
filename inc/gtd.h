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
  HELLO = 2,
  SYNC_UTXO  = 3,
  EXPAND_CONNECTION = 4,
  REQ_IPS = 5,
  REQ_HASHSET_FILTER = 6,
  REQ_HASHSET = 7,
  SYNC_HASHSET = 8
};

struct Task {
  TaskType type;
  Bytes data;
  std::time_t time;

Task(TaskType type_) : type(type_), time(0){}
Task(TaskType type_, std::time_t time_) : type(type_), time(time_){}
Task(TaskType type_, Bytes &b, std::time_t time_ = 0) : type(type_), data(b), time(time_){}
};

bool operator<(Task const&l, Task const &r) {
  if (l.time == r.time)
    return l.type < r.type;
  return l.time < r.time;
}

bool operator>(Task const&l, Task const &r) {
  if (l.time == r.time)
    return l.type > r.type;
  return l.time > r.time;
}

struct GTD {
  std::priority_queue<Task, std::vector<Task>, std::greater<Task> > q;
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

  void operator()(Task task) {
    add(task);
  }
  
  void add(Task task) {
    task.time = 0;
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
