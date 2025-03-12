#include "pch.h"
#include "ConnectionFactory.h"

class TCPConnection final : public ConnectionFactory::Connection {
public:
  TCPConnection(const ConnectionFactory::ConnectionConfig& cfg): Connection(cfg) {
    _socket_fd = cfg.low_latency ? create_low_latency_socket() : create_standard_socket();
  }

  bool send(const Order& order) override { (void)order;
    return true;    //serialize and send via socket
  }

  bool is_healthy() const override { return _socket_fd > 0; }
  uint64_t last_latency_ns() const override { return _last_latency_ns; }

private:
  int create_low_latency_socket() {
    return 1; //dummy fd, would set TCP_NODELAY, SO_PRIORITY, etc.
  }

  int create_standard_socket() { return 2; } // dummy fd

  int _socket_fd = -1;
  uint64_t _last_latency_ns = 50000; // 50 microsecs
};

class SharedMemConnection final : public ConnectionFactory::Connection {
public:
  SharedMemConnection(const ConnectionFactory::ConnectionConfig& cfg) : Connection(cfg) {}  //map shared memory region

  bool send(const Order& order) override { (void)order;  //write to shared memory
    return true;
  }

  bool is_healthy() const override { return true; }
  uint64_t last_latency_ns() const override { return 500; } // 500ns - much faster

private:
  void* _shm_ptr = nullptr;
};

ConnectionFactory::ConnectionPtr ConnectionFactory::create(const ConnectionConfig& config) {
  auto key = make_cache_key(config);    // Try to find in cache first
  {
    std::shared_lock lock(_cache_mutex);
    auto it = _connection_cache.find(key);

    if (it != _connection_cache.end()) {
      auto conn = it->second.lock();
    
      if (conn && conn->is_healthy()) return conn;
    }
  }

  ConnectionPtr conn;

  switch (config.protocol) {
  case Protocol::TCP:
  case Protocol::FIX:
    conn = std::make_shared<TCPConnection>(config);
    break;
  case Protocol::SHARED_MEM:
    conn = std::make_shared<SharedMemConnection>(config);
    break;
  default:
    conn = std::make_shared<TCPConnection>(config); // fallback
  }

  {  // Store in cache
    std::unique_lock lock(_cache_mutex);
    _connection_cache[key] = conn;
  }

  return conn;
}

ConnectionFactory::ConnectionPtr ConnectionFactory::best_for_venue(Exchange venue, bool ultra_low_latency) {
  ConnectionConfig cfg{};
  cfg.venue = venue;

  if (venue == Exchange::NYSE || venue == Exchange::NASDAQ) {
    if (ultra_low_latency) {
      cfg.protocol = Protocol::SHARED_MEM;
      cfg.low_latency = true;
      cfg.port = 0;
    }
    else {
      cfg.protocol = Protocol::FIX;
      cfg.low_latency = true;
      cfg.port = 8001;
    }
  }
  else {
    cfg.protocol = Protocol::TCP;
    cfg.low_latency = true;
    cfg.port = 9001;
  }

  return create(cfg);
}

uint64_t ConnectionFactory::make_cache_key(const ConnectionConfig& cfg) {
  return (static_cast<uint64_t>(cfg.venue      ) << 24)
       | (static_cast<uint64_t>(cfg.protocol   ) << 16)
       | (static_cast<uint64_t>(cfg.low_latency) <<  8)
       | (static_cast<uint64_t>(cfg.port       ) <<  0);
}
