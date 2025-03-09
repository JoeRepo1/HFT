#include <array>
#include <cmath>
#include <concepts>
#include <condition_variable>
#include <cstdint>
#include <execution>
#include <filesystem>
#include <format>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <numeric>
#include <queue>
#include <random>
#include <ranges>
#include <tuple>
#include <utility>

#include "AsyncLogger.h"
#include "AlignedBuffer.h"
#include "ConnectionFactory.h"

class Trading {
public:
  Trading() : logger("trading_log.txt") {}

  void run() { hybridAlgo(); }

private:
  AsyncLogger logger;

  double current_exposure = 0.0;
  constexpr static double alpha = 0.3;
  constexpr static double min_weight = 0.05;
  constexpr static double max_order_size = 1000.0;
  constexpr static double max_exposure = 10000.0;
  constexpr static double vol_limit = 2.0;

  constexpr static double omega = 0.00001;
  constexpr static double alpha_garch = 0.1;
  constexpr static double beta_garch = 0.8;
  double prev_volatility = 0.01;
  inline static std::atomic<uint64_t> order_id_counter = 0;

  using SignalStrategy = std::function<double(const MarketFeatures&)>;

  const std::vector<SignalStrategy> strategies = {
      [this](const auto& f) { return pairTradingSignal(f); },
      [this](const auto& f) { return microstructureAlphaSignal(f); },
      [this](const auto& f) { return orderBookImbalanceSignal(f); },
      [this](const auto& f) { return momentumIgnitionSignal(f); },
      [this](const auto& f) { return volatilityBreakoutSignal(f); },
      [this](const auto& f) { return hotPatternA(f); }
  };

  AlignedBuffer<double> ewma_pnl{ strategies.size() };
  AlignedBuffer<double> signals_buff{ strategies.size() };

  void hybridAlgo() {
    MarketFeatures features = computeFeatures();

    L("Computed features: volatility=", features.volatility, ", trend_strength=", features.trend_strength);

    double confidence_threshold = std::max(0.05, 0.1 * features.volatility);

    auto compute_signal = [&features](const auto& strategy) { return strategy(features); };

    auto future_pair_signal = std::async(std::launch::async, compute_signal, strategies[0]);

    using namespace std::ranges;
    transform(subrange(strategies.begin() + 1, strategies.end()), signals_buff.data() + 1, compute_signal);

    signals_buff[0] = future_pair_signal.get();

    updateEWMA(signals_buff.span());

    auto [total_weight, blended_signal] = blendSignals(signals_buff.span(), confidence_threshold);

    if (total_weight > 0) {
      blended_signal /= total_weight;

      L("Blended signal: ", blended_signal, " with total weight: ", total_weight);

      double potential_exposure = std::abs(current_exposure) + std::abs(blended_signal) * 100;

      if (potential_exposure <= max_exposure && std::abs(blended_signal) > 0.01) {
        auto [px, qty, filled] = smartExecute(blended_signal, features);

        if (filled) {
          updateExposure(blended_signal > 0, px, qty);
          L("Order executed: price=", px, ", qty=", qty, ", side=", (blended_signal > 0 ? 'B' : 'S'));
        }
        else
          L("Order not executed: price=", px, ", qty=", qty);
      }
    }
    else
      L("No tradable signal detected");
  }

  MarketFeatures computeFeatures() {
    static constexpr size_t kWindowSize = 100;
    static std::array<double, kWindowSize> returns_buffer{};
    static size_t buffer_pos = 0;

    double return_t = getLatestReturn();
    returns_buffer[buffer_pos++ % kWindowSize] = return_t;

    double volatility = computeGarchVolatility(return_t);
    double trend = detectTrend(std::span(returns_buffer));
    double liquidity = estimateLiquidity();
    auto [bid_ask, imbalance] = getOrderBookMetrics();

    return {
        .volatility = volatility,
        .trend_strength = trend,
        .order_book_imbalance = imbalance,
        .cointegration_zscore = computeCointegration(),
        .price_impact = estimatePriceImpact(liquidity),
        .liquidity_score = liquidity,
        .short_term_reversal = detectShortTermReversal(),
        .bid_ask_spread = bid_ask
    };
  }

  double computeGarchVolatility(double return_t) {
    double variance = omega + alpha_garch * return_t * return_t + beta_garch * prev_volatility * prev_volatility;
    double volatility = std::sqrt(variance);
    prev_volatility = std::clamp(volatility, 0.0001, 5.0);

    return volatility;
  }

  double detectTrend(std::span<const double> returns) {
    if (returns.empty()) return 0.0;

    double sum = 0.0, weight = 1.0, total_weight = 0.0;

    for (auto it = returns.rbegin(); it != returns.rend(); ++it) {
      sum += *it * weight;
      total_weight += weight;
      weight *= 0.95;
    }

    return sum / total_weight;
  }

  double pairTradingSignal(const MarketFeatures& f) {
    return (f.cointegration_zscore > 2.0) ? -1.0 : (f.cointegration_zscore < -2.0) ? 1.0 : 0.0;
  }

  double microstructureAlphaSignal(const MarketFeatures& f) {
    double signal = 0.0;

    if (f.bid_ask_spread < 0.02 && f.order_book_imbalance > 0.5)
      signal = 1.0 * std::exp(-5.0 * f.price_impact);
    else if (f.bid_ask_spread < 0.02 && f.order_book_imbalance < -0.5)
      signal = -1.0 * std::exp(-5.0 * f.price_impact);

    return signal;
  }

  double orderBookImbalanceSignal(const MarketFeatures& f) {
    double impact_adjusted = f.order_book_imbalance * (1.0 - f.price_impact * 10.0);

    return (impact_adjusted > 0.3) ? 1.0 : (impact_adjusted < -0.3) ? -1.0 : 0.0;
  }

  double momentumIgnitionSignal(const MarketFeatures& f) {
    double combined = f.trend_strength * (1.0 + f.liquidity_score);

    return (combined > 0.7) ? 1.0 : (combined < -0.7) ? -1.0 : 0.0;
  }

  double volatilityBreakoutSignal(const MarketFeatures& f) {
    double threshold = 1.5 * prev_volatility;

    return (f.volatility > threshold && f.short_term_reversal < -0.2) ? 1.0 :
           (f.volatility < threshold * 0.3 && f.short_term_reversal > 0.2) ? -1.0 : 0.0;
  }

  double hotPatternA(const MarketFeatures& f) {
    double signal = 0.0;

    bool tight_spread_high_vol = (f.bid_ask_spread < 0.01) && (f.volatility > 1.5 * prev_volatility);
    bool contra_flow = (f.order_book_imbalance > 0.4 && f.short_term_reversal < -0.3) ||
                       (f.order_book_imbalance < -0.4 && f.short_term_reversal > 0.3);
    bool hidden_liquidity = (f.liquidity_score < 0.4) && (f.price_impact < 0.02);

    if (tight_spread_high_vol && contra_flow && hidden_liquidity) {
      signal = (f.cointegration_zscore > 0) ? -2.0 : 2.0;

      if (std::abs(f.trend_strength) < 0.2) signal *= 1.5;

      L("HotPatternA detected! Signal strength: ", signal);
    }

    return signal;
  }

  void updateEWMA(std::span<const double> signals) {
    if (has_avx2()) {
      for (size_t i = 0; i < signals.size() - 3; i += 4) {
        __m256d signals_vec = _mm256_loadu_pd(&signals[i]);
        __m256d pnl_vec = _mm256_mul_pd(signals_vec, _mm256_set1_pd(0.1));
        __m256d ewma_vec = _mm256_loadu_pd(&ewma_pnl[i]);
        __m256d alpha_vec = _mm256_set1_pd(alpha);
        __m256d one_minus_alpha = _mm256_set1_pd(1.0 - alpha);
        __m256d result = _mm256_add_pd(_mm256_mul_pd(alpha_vec, pnl_vec),
          _mm256_mul_pd(one_minus_alpha, ewma_vec));
        _mm256_storeu_pd(&ewma_pnl[i], result);
      }

      for (size_t i = (signals.size() / 4) * 4; i < signals.size(); ++i)
        ewma_pnl[i] = alpha * (signals[i] * 0.1) + (1.0 - alpha) * ewma_pnl[i];
    }
    else
      for (size_t i = 0; i < signals.size(); ++i)
        ewma_pnl[i] = alpha * (signals[i] * 0.1) + (1.0 - alpha) * ewma_pnl[i];
  }

  struct BlendResult { double total_weight; double blended_signal; };

  BlendResult blendSignals(std::span<const double> signals, double confidence_threshold) {
    double total_weight = 0.0, blended_signal = 0.0;

    if (has_avx2()) {
      __m256d total_weight_vec = _mm256_setzero_pd();
      __m256d blended_signal_vec = _mm256_setzero_pd();

      for (size_t i = 0; i < signals.size() - 3; i += 4) {
        __m256d signals_vec = _mm256_loadu_pd(&signals[i]);
        __m256d ewma_vec = _mm256_loadu_pd(&ewma_pnl[i]);
        __m256d thresh_vec = _mm256_set1_pd(confidence_threshold);
        __m256d min_vec = _mm256_set1_pd(min_weight);
        __m256d abs_signals = _mm256_andnot_pd(_mm256_set1_pd(-0.0), signals_vec);
        __m256d mask = _mm256_and_pd(
          _mm256_cmp_pd(ewma_vec, min_vec, _CMP_GT_OQ),
          _mm256_cmp_pd(abs_signals, thresh_vec, _CMP_GT_OQ)
        );

        __m256d weights = _mm256_and_pd(ewma_vec, mask);
        __m256d weighted_signals = _mm256_mul_pd(weights, signals_vec);

        total_weight_vec = _mm256_add_pd(total_weight_vec, weights);
        blended_signal_vec = _mm256_add_pd(blended_signal_vec, weighted_signals);
      }

      double weight_arr[4], signal_arr[4];
      _mm256_storeu_pd(weight_arr, total_weight_vec);
      _mm256_storeu_pd(signal_arr, blended_signal_vec);
      total_weight = weight_arr[0] + weight_arr[1] + weight_arr[2] + weight_arr[3];
      blended_signal = signal_arr[0] + signal_arr[1] + signal_arr[2] + signal_arr[3];

      for (size_t i = (signals.size() / 4) * 4; i < signals.size(); ++i)
        if (ewma_pnl[i] > min_weight && std::abs(signals[i]) > confidence_threshold) {
          total_weight += ewma_pnl[i];
          blended_signal += ewma_pnl[i] * signals[i];
        }
    }
    else
      for (size_t i = 0; i < signals.size(); ++i)
        if (ewma_pnl[i] > min_weight && std::abs(signals[i]) > confidence_threshold) {
          total_weight += ewma_pnl[i];
          blended_signal += ewma_pnl[i] * signals[i];
        }

    return { total_weight, blended_signal };
  }

  std::unordered_map<Exchange, ConnectionFactory::ConnectionPtr> _venue_connections;
  Symbols symbols;

  bool sendOrder(const Order& order) {
    auto& conn = _venue_connections[order.exchange];

    if (!conn || !conn->is_healthy()) {
      bool ultra_low = (order.symbol == symbols.AAPL || order.symbol == symbols.TSLA);

      conn = ConnectionFactory::best_for_venue(order.exchange, ultra_low);
    }

    return conn->send(order);
  }

  std::tuple<double, int, bool> smartExecute(double signal, const MarketFeatures& features) {
    double base_size = std::abs(signal) * 100;
    double adjusted_size = base_size / (1.0 + std::abs(current_exposure) / max_exposure);
    
    if (features.liquidity_score < 0.3) adjusted_size *= 0.5;
    
    double size = std::clamp(adjusted_size * std::exp(-5.0 * features.price_impact), 1.0, max_order_size);
    int qty = static_cast<int>(size);
    double price = 100.0 * (1.0 + (signal > 0 ? features.bid_ask_spread : -features.bid_ask_spread));
    char side = (signal > 0) ? 'B' : 'S';
    
    Order order {
      .exchange = Exchange::NYSE,  //ph
      .side = side,
      .symbol = symbols.AAPL,      //ph
      .order_id = order_id_counter.fetch_add(1, std::memory_order_relaxed),
      .price = price,
      .quantity = qty,
    };

    if (limitCheck(price, qty) && sendOrder(order)) return { price, qty, true };

    return { price, qty, false };
  }

  void updateExposure(bool is_buy, double price, int quantity) {
    current_exposure += (is_buy ? 1.0 : -1.0) * price * quantity;
  }

  std::pair<double, double> getOrderBookMetrics() { return { 0.01, 0.2 };            }
  double computeCointegration()                   { return -1.5;                     }
  double estimateLiquidity()                      { return 0.7;                      }
  double estimatePriceImpact(double liquidity)    { return 0.01 / (liquidity + 0.1); }
  double detectShortTermReversal()                { return -0.3;                     }
  double getLatestReturn()                        { return 0.001;                    }
  
  bool limitCheck(double price, int qty) {
    return std::abs(current_exposure) + price * qty <= max_exposure && price * qty <= vol_limit * 10000.0;
  }

  bool has_avx2() const {
#if defined(_MSC_VER)
    int cpuInfo[4];
    __cpuid(cpuInfo, 0);

    if (cpuInfo[0] >= 7) {
      __cpuidex(cpuInfo, 7, 0);
      return (cpuInfo[1] & (1 << 5)) != 0;
    }
#elif defined(__GNUC__) || defined(__clang__)
    unsigned int eax, ebx, ecx, edx;

    if (__get_cpuid(0, &eax, &ebx, &ecx, &edx) && eax >= 7) {
      __get_cpuid_count(7, 0, &eax, &ebx, &ecx, &edx);
      return (ebx & (1 << 5)) != 0;
    }
#endif

    return false;
  }
};

int main() {
  Trading trading;
  trading.run();

  std::cout << "Enter to continue.\n";
  std::cin.get();
}
