#include "AsyncLogger.h"

AsyncLogger::AsyncLogger(const std::string& filename): log_file(filename, std::ios::app), head(0), tail(0), stop_flag(false) {
  if (!log_file) {
    std::cerr << "Failed to open log file: " << filename << std::endl;
    return;
  }

  log_thread = std::thread(&AsyncLogger::processLogs, this);
}

AsyncLogger::~AsyncLogger() {
  stop_flag.store(true);
  
  if (log_thread.joinable()) log_thread.join();
  
  flushLogs();
}

void AsyncLogger::push(std::string&& log_entry) {
  size_t next = (head.load(std::memory_order_relaxed) + 1) % LOG_QUEUE_SIZE;
  
  if (next == tail.load(std::memory_order_acquire)) return;  // Drop log if full (non-blocking)

  buffer[head.load(std::memory_order_relaxed)] = std::move(log_entry);
  
  head.store(next, std::memory_order_release);
}

bool AsyncLogger::pop(std::string& log_entry) {
  if (tail.load(std::memory_order_acquire) == head.load(std::memory_order_relaxed)) return false;

  log_entry = std::move(buffer[tail.load(std::memory_order_relaxed)]);

  tail.store((tail.load(std::memory_order_relaxed) + 1) % LOG_QUEUE_SIZE, std::memory_order_release);

  return true;
}

void AsyncLogger::processLogs() {
  std::string log_entry;

  while (!stop_flag.load(std::memory_order_acquire)) {
    while (pop(log_entry)) log_file << log_entry;

    std::this_thread::sleep_for(std::chrono::milliseconds(1));  // Avoid busy-wait
  }

  flushLogs();
}

void AsyncLogger::flushLogs() {
  std::string log_entry;

  while (pop(log_entry)) log_file << log_entry;
}

std::string AsyncLogger::getTimestamp() {
  using namespace std::chrono;

  auto now = system_clock::now();
  auto now_time_t = system_clock::to_time_t(now);
  auto now_ns = duration_cast<nanoseconds>(now.time_since_epoch()) % 1'000'000'000;
  std::tm local_tm;
  std::ostringstream timestamp;

#ifdef _MSC_VER
  localtime_s(&local_tm, &now_time_t);
#else
  localtime_r(&now_time_t, &local_tm);
#endif
  
  timestamp << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S") << "." << std::setw(9) << std::setfill('0') << now_ns.count();

  return timestamp.str();
}
