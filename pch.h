#pragma once  //pch.h

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
#include <algorithm>
#include <shared_mutex>
#include <unordered_map>
#include <algorithm>
#include <span>
#include <stdexcept>
#include <type_traits>
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

#ifdef _MSC_VER
#include <malloc.h>
#include <intrin.h>
#else
#include <cstdlib>
#include <cpuid.h>
#include <x86intrin.h>
#endif

#include "InitGlobals.h"
