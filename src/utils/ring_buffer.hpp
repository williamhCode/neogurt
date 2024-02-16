#pragma once

#include <vector>
#include <cassert>

// ring buffer for optimzed scrolling
template <typename T>
class RingBuffer {
private:
  std::vector<T> buffer;
  size_t head = 0;
  size_t _size;

public:
  // Iterator class
  class Iterator {
  private:
    RingBuffer* rb;
    size_t index;

  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = T*;
    using reference = T&;

    Iterator(RingBuffer* rb, size_t index) : rb(rb), index(index) {
    }

    reference operator*() {
      return (*rb)[index];
    }

    pointer operator->() {
      return &((*rb)[index]);
    }

    // Prefix increment
    Iterator& operator++() {
      index++;
      if (index >= rb->_size) {
        index = 0;
      }
      return *this;
    }

    // Postfix increment
    Iterator operator++(int) {
      Iterator tmp = *this;
      ++(*this);
      return tmp;
    }

    friend bool operator==(const Iterator& a, const Iterator& b) {
      return a.index == b.index;
    }

    friend bool operator!=(const Iterator& a, const Iterator& b) {
      return a.index != b.index;
    }
  };

  RingBuffer() = default;

  RingBuffer(size_t size) : buffer(size), _size(size) {
  }

  T& operator[](size_t index) {
    assert(index < _size);
    size_t actualIndex = (head + index) % _size;
    return buffer[actualIndex];
  }

  const T& operator[](size_t index) const {
    assert(index < _size);
    size_t actualIndex = (head + index) % _size;
    return buffer[actualIndex];
  }

  void scrollUp(size_t lines) {
    head = (head + lines) % _size;
  }

  void scrollDown(size_t lines) {
    head = (head + _size - lines) % _size;
  }

  size_t size() const {
    return _size;
  }

  void resize(size_t newCapacity) {
  }

  Iterator begin() {
    return Iterator(this, 0);
  }

  Iterator end() {
    return Iterator(this, _size);
  }
};
