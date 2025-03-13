#pragma once  //pch.h

//------ Trading
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <execution>
#include <fstream>
#include <functional>
#include <future>
#include <immintrin.h>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <span>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <vector>
#include <xmmintrin.h>

#if defined(_MSC_VER)
#include <intrin.h>
#elif defined(__GNUC__) || defined(__clang__)
#include <cpuid.h>
#endif

using std::abs, std::async, std::clamp, std::exp, std::make_unique, std::max, std::sqrt, std::tanh;
using std::cin, std::cout, std::memory_order_relaxed, std::ranges::transform;
using std::array, std::atomic, std::function, std::launch, std::pair, std::ranges::subrange,
      std::span, std::tuple, std::unique_ptr, std::unordered_map, std::vector;

//------ ConnectionFactory
#include <cstdint>
#include <memory>
#include <shared_mutex>
#include <unordered_map>

//------ AsyncLogger
#include <atomic>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

//------ AI
#include <filesystem>
#include <string>
#include <utility>

//------ AlignedBuffer
#include <algorithm>
#include <cstddef>
#include <memory>
#include <span>
#include <stdexcept>

using std::fill_n, std::swap;
using std::is_arithmetic_v;
using std::bad_alloc;

//------ Globals
#include "InitGlobals.h"
