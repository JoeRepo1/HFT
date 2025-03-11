#pragma once  //AsyncLogger.h

#include <atomic>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#define L logger.log

class AsyncLogger {
public:
  explicit AsyncLogger(const std::string& filename);
  ~AsyncLogger();

  template <typename... Args> void log(Args&&... args) {
    std::ostringstream log_entry;

    log_entry << getTimestamp() << " ", (log_entry << ... << std::forward<Args>(args)) << "\n";

    push(log_entry.str());
  }

private:
  std::ofstream log_file;
  std::thread log_thread;
  std::atomic<bool> stop_flag;
  static constexpr size_t LOG_QUEUE_SIZE = 1024;

  std::vector<std::string> buffer{ LOG_QUEUE_SIZE };  //lock-free ring buffer
  std::atomic<size_t> head, tail;

  void push(std::string&& log_entry);   //non-blocking enqueue
  bool pop(std::string& log_entry);   //blocking dequeue
  void processLogs();
  void flushLogs();
  std::string getTimestamp();
};
