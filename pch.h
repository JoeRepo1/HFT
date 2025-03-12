#pragma once  //pch.h

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cmath>
#include <concepts>
#include <condition_variable>
#include <cstdint>
#include <ctime>
#include <execution>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <future>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <numeric>
#include <queue>
#include <random>
#include <ranges>
#include <shared_mutex>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#ifdef _MSC_VER
#include <intrin.h>
#include <malloc.h>
#else
#include <cpuid.h>
#include <cstdlib>
#include <x86intrin.h>
#endif

#include "InitGlobals.h"

//trading.cpp
using std::abs, std::async, std::clamp, std::exp, std::make_unique, std::max, std::sqrt, std::tanh;
using std::cin, std::cout, std::memory_order_relaxed, std::ranges::transform;
using std::array, std::atomic, std::function, std::launch, std::pair, std::ranges::subrange,
      std::span, std::tuple, std::unique_ptr, std::unordered_map, std::vector;
