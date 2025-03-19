#pragma once  //AsyncLogger.h

#define L logger.log

struct alignas(64) AtomicCounter : public std::atomic<size_t> {
  using std::atomic<size_t>::load;
  using std::atomic<size_t>::store;

  AtomicCounter(): std::atomic<size_t>(0), line_padding("") {}

  const char line_padding[(64 - sizeof(std::atomic<size_t>) % 64) % 64];
};

class AsyncLogger {
public:
  explicit AsyncLogger(const std::string& filename);
  ~AsyncLogger();

  template <typename... Args> void log(Args&&... args) {
    size_t current_head = head.load(std::memory_order_relaxed);

    while (true) {
      size_t next = (current_head + 1) & LOG_QUEUE_MASK;
      size_t current_tail = tail.load(std::memory_order_acquire);

      if (next == current_tail) {
        dropped_logs.fetch_add(1, std::memory_order_relaxed);
        return;
      }

      if (head.compare_exchange_weak(current_head, next, std::memory_order_release))
        break;
    }

    std::ostringstream& log_entry = entry_buffers[current_head];

#if defined(_MSC_VER)
    _mm_prefetch(reinterpret_cast<const char*>(&log_entry), _MM_HINT_T0);
#elif defined(__GNUC__) || defined(__clang__)
    __builtin_prefetch(&log_entry);
#endif

    log_entry.str({});
    log_entry.clear();

    log_entry << getTimestamp() << " ";
    (log_entry << ... << std::forward<Args>(args)) << "\n";

    has_data.store(true, std::memory_order_relaxed);
  }

  void signalFlush();
  void notifyHighPriority(bool bStart);
  size_t getDroppedCount() const;

private:
  AtomicCounter head, tail;

  std::ofstream log_file;
  std::thread log_thread;
  std::atomic<bool> stop_flag{ false };
  std::atomic<bool> high_priority_operation{ false };
  std::atomic<bool> has_data{ false };
  std::atomic<bool> needs_flush{ false };
  std::atomic<size_t> dropped_logs{ 0 };

  static constexpr size_t LOG_QUEUE_SIZE = 0x2000;
  static constexpr size_t LOG_QUEUE_MASK = LOG_QUEUE_SIZE - 1;

  std::vector<std::ostringstream> entry_buffers{ LOG_QUEUE_SIZE };

  void processLogs();
  void flushLogs();
  std::string getTimestamp();

  AsyncLogger(const AsyncLogger&) = delete;
  AsyncLogger& operator=(const AsyncLogger&) = delete;
};
