#pragma once

#ifdef _MSC_VER
#include <intrin.h>
#else
#include <cpuid.h>
#include <x86intrin.h>
#endif

enum class Exchange : uint8_t { NYSE, NASDAQ, CME };

struct alignas(64) Order {
  Exchange exchange;        
  char side;                
  uint16_t padding;        
  uint32_t symbol;          
  uint64_t order_id;        
  double price;             
  int32_t quantity;         
  uint32_t timestamp;       //latency tracking
  uint64_t strategy_id;     
  uint64_t target_venue_ns; //execution timing
  char cache_line_padding[16];  //prevent false sharing with adjacent objects

  void prefetch() const {
#if defined(_MSC_VER)
    _mm_prefetch(reinterpret_cast<const char*>(this), _MM_HINT_T0);
#elif defined(__GNUC__) || defined(__clang__)
    __builtin_prefetch(this, 0, 3);
#endif
  }
};

static_assert(sizeof(Order) == 64, "Order struct shd be 1 cache line (64 bytes)");

struct MarketFeatures {
  double volatility;
  double trend_strength;
  double order_book_imbalance;
  double cointegration_zscore;
  double price_impact;
  double liquidity_score;
  double short_term_reversal;
  double bid_ask_spread;
};

struct Symbols {
  const uint32_t    //freq traded
    AAPL = hashSymbol("AAPL"),
    TSLA = hashSymbol("TSLA"),
    ES   = hashSymbol("ES");

  uint32_t hashSymbol(const char* str) noexcept {
    constexpr uint32_t fnv_prime    = 0x01000193;  //  16777619
    constexpr uint32_t offset_basis = 0x811C9DC5;  //2166136261
    uint32_t hash;

    for (hash = offset_basis; *str; ++str) {
      hash ^= static_cast<uint32_t>(*str);
      hash *= fnv_prime;
    }

    return hash;
  }
};
