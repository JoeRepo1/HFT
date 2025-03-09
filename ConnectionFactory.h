#pragma once

#include <shared_mutex>
#include <unordered_map>
#include <memory>

#include "InitGlobals.h"

class ConnectionFactory {
public:
  enum class Protocol : uint8_t { TCP, UDP, FIX, MULTICAST, SHARED_MEM };

  struct ConnectionConfig {
    Exchange venue;
    Protocol protocol;
    bool low_latency;
    uint16_t port;
  };

  class Connection {  // Base connection class (abstract)
  public:
    virtual ~Connection() = default;
    virtual bool send(const Order& order) = 0;
    virtual bool is_healthy() const = 0;
    virtual uint64_t last_latency_ns() const = 0;

    const ConnectionConfig& config() const { return _config; }    // Expose configuration

  protected:
    Connection(const ConnectionConfig& cfg) : _config(cfg) {}
    ConnectionConfig _config;
  };

  using ConnectionPtr = std::shared_ptr<class Connection>;

  static ConnectionPtr create(const ConnectionConfig& config);

  static ConnectionPtr best_for_venue(Exchange venue, bool ultra_low_latency = false);

private:
  static uint64_t make_cache_key(const ConnectionConfig& cfg);

  inline static std::shared_mutex _cache_mutex;
  inline static std::unordered_map<uint64_t, std::weak_ptr<Connection>> _connection_cache;
};
