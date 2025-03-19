#include "pch.h"
#include "AsyncLogger.h"

void AsyncLogger::signalFlush() {
  needs_flush.store(true, std::memory_order_release);
}

void AsyncLogger::notifyHighPriority(bool bStart) {
  high_priority_operation.store(bStart, std::memory_order_release);
}

size_t AsyncLogger::getDroppedCount() const {
  return dropped_logs.load(std::memory_order_relaxed);
}

AsyncLogger::AsyncLogger(const std::string& filename) : log_file(filename, std::ios::app) {
  if (!log_file) throw std::runtime_error("Failed to open log file: " + filename);

  log_thread = std::thread(&AsyncLogger::processLogs, this);

#ifdef _WIN32
  SetThreadPriority(log_thread.native_handle(), THREAD_PRIORITY_BELOW_NORMAL);
#elif defined(__GNUC__) || defined(__clang__)
  sched_param param;
  param.sched_priority = 0;
  pthread_setschedparam(log_thread.native_handle(), SCHED_OTHER, &param);

  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(std::thread::hardware_concurrency() - 1, &cpuset);
  pthread_setaffinity_np(log_thread.native_handle(), sizeof(cpu_set_t), &cpuset);
#endif
}

AsyncLogger::~AsyncLogger() {
  stop_flag.store(true, std::memory_order_release);
  if (log_thread.joinable()) log_thread.join();

  processLogs();
  flushLogs();

  size_t dropped = dropped_logs.load(std::memory_order_relaxed);

  if (dropped > 0) log_file << "Dropped " << dropped << " log messages due to full buffer.\n";

  log_file.flush();
}

void AsyncLogger::processLogs() {
  const auto spin_yield_threshold = 1000;
  const auto sleep_threshold = 10000;
  const auto min_sleep = std::chrono::microseconds(1);
  const auto max_sleep = std::chrono::microseconds(100);

  auto current_sleep = min_sleep;
  int idle_iterations = 0;
  size_t avg_batch = 32;

  while (!stop_flag.load(std::memory_order_relaxed)) {
    if (high_priority_operation.load(std::memory_order_acquire)) {
      std::this_thread::sleep_for(min_sleep);
      continue;
    }

    if (!has_data.load(std::memory_order_relaxed) && !needs_flush.load(std::memory_order_acquire)) {
      idle_iterations++;

      if (idle_iterations < spin_yield_threshold) std::this_thread::yield();
      else if (idle_iterations < sleep_threshold) std::this_thread::yield();
      else {
        std::this_thread::sleep_for(current_sleep);
        if (current_sleep < max_sleep) current_sleep += std::chrono::microseconds(1);
      }

      continue;
    }

    idle_iterations = 0;
    current_sleep = min_sleep;

    size_t current_tail = tail.load(std::memory_order_relaxed);
    const size_t current_head = head.load(std::memory_order_acquire);

    if (current_tail != current_head) {
      size_t available = (current_head >= current_tail) ? (current_head - current_tail) : (LOG_QUEUE_SIZE - current_tail + current_head);
      size_t batch_size = min(max(32, static_cast<int>(available / 4)), static_cast<int>(available));
      size_t processed = 0;

      while (current_tail != current_head && processed < batch_size) {
        try {
          log_file << std::string_view(entry_buffers[current_tail].str());
        }
        catch (...) {}

        current_tail = (current_tail + 1) % LOG_QUEUE_SIZE;
        processed++;
      }

      avg_batch = (avg_batch * 7 + processed * 3) / 10;
      tail.store(current_tail, std::memory_order_release);

      if (current_tail == current_head)
        has_data.store(false, std::memory_order_relaxed);
    }

    if (needs_flush.load(std::memory_order_acquire) || (current_tail == current_head && log_file.tellp() > 0)) {
      log_file.flush();
      needs_flush.store(false, std::memory_order_release);
    }
  }
}

void AsyncLogger::flushLogs() {
  size_t current_tail = tail.load(std::memory_order_relaxed);
  const size_t current_head = head.load(std::memory_order_acquire);

  while (current_tail != current_head) {
    try {
      log_file << std::string_view(entry_buffers[current_tail].str());
    }
    catch (...) {}

    current_tail = (current_tail + 1) % LOG_QUEUE_SIZE;
  }

  tail.store(current_tail, std::memory_order_release);
  log_file.flush();
}

std::string AsyncLogger::getTimestamp() {
  using namespace std::chrono;

  static thread_local auto last_time_point = system_clock::now();
  static thread_local std::string last_timestamp;

  auto now = system_clock::now();

  if (last_time_point != now || last_timestamp.empty()) {
    auto now_time_t = system_clock::to_time_t(now);
    auto now_ns = duration_cast<nanoseconds>(now.time_since_epoch()) % 1'000'000'000;
    std::tm local_tm;

#ifdef _MSC_VER
    localtime_s(&local_tm, &now_time_t);
#else
    localtime_r(&now_time_t, &local_tm);
#endif

    std::ostringstream timestamp;

    timestamp << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S") << "."  << std::setw(9) << std::setfill('0') << now_ns.count();
    
    last_timestamp = timestamp.str();
    last_time_point = now;
  }

  return last_timestamp;
}
