#pragma once  //AlignedBuffer.h

#include <algorithm>
#include <span>
#include <stdexcept>
#include <type_traits>

#ifdef _MSC_VER
#include <malloc.h>
#else
#include <cstdlib>
#endif

template <typename T> concept Numeric = std::is_arithmetic_v<T>;
template <Numeric T> class AlignedBuffer {
public:
  AlignedBuffer(size_t size) : size_(size) {
    data_ = static_cast<T*>(_aligned_malloc(size * sizeof(T), 32));

    if (!data_) throw std::bad_alloc();

    std::fill_n(data_, size, T{});
  }

  ~AlignedBuffer() { _aligned_free(data_); }

  std::span<T> span() { return std::span<T>(data_, size_); }
  std::span<const T> span() const { return std::span<const T>(data_, size_); }
  T& operator[](size_t i) { return data_[i]; }
  const T& operator[](size_t i) const { return data_[i]; }
  T* data() { return data_; }
  const T* data() const { return data_; }

  AlignedBuffer(AlignedBuffer&& other) noexcept : data_(nullptr), size_(0) {
    std::swap(data_, other.data_);
    std::swap(size_, other.size_);
  }

  AlignedBuffer& operator=(AlignedBuffer&& other) noexcept {
    if (this != &other) {
      std::swap(data_, other.data_);
      std::swap(size_, other.size_);
    }

    return *this;
  }

private:
  T* data_;
  size_t size_;
};
