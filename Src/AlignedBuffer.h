#pragma once  //AlignedBuffer.h

template <typename T> concept Numeric = is_arithmetic_v<T>;
template <Numeric T> class AlignedBuffer {
public:
  AlignedBuffer(size_t size) : _size(size) {
    _data = static_cast<T*>(_aligned_malloc(size * sizeof(T), 32));

    if (!_data) throw bad_alloc();

    fill_n(_data, size, T{});
  }

  ~AlignedBuffer() { _aligned_free(_data); }

  std::span<T> span()                 { return std::span<T>(_data, _size); }
  std::span<const T> span() const     { return std::span<const T>(_data, _size); }

  T& operator[](size_t i)             { return _data[i]; }
  const T& operator[](size_t i) const { return _data[i]; }
  
  T* data()                           { return _data; }
  const T* data() const               { return _data; }

  AlignedBuffer(AlignedBuffer&& other) noexcept : _data(nullptr), _size(0) {
    swap(_data, other._data);
    swap(_size, other._size);
  }

  AlignedBuffer& operator=(AlignedBuffer&& other) noexcept {
    if (this != &other) {
      swap(_data, other._data);
      swap(_size, other._size);
    }

    return *this;
  }

private:
  T* _data;
  size_t _size;
};
