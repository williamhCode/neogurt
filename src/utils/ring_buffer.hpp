#pragma once

#include <vector>
#include <cassert>

// ring buffer for optimzed scrolling
template <typename T>
class RingBuffer {
private:
  std::vector<T> buffer;
  size_t head = 0;
  size_t size;

  size_t wrapIndex(size_t index) const {
    return index >= size ? index - size : index;
  }

public:
  // Iterator class
  class Iterator {
  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = T*;
    using reference = T&;

    Iterator(pointer ptr, size_t size, size_t index)
        : ptr(ptr), size(size), index(index) {
    }

    reference operator*() {
      return *(ptr + index);
    }

    pointer operator->() {
      return ptr + index;
    }

    // Prefix increment
    Iterator& operator++() {
      index++;
      if (index >= size) {
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

  private:
    pointer ptr;
    size_t size;
    size_t index;
  };

  RingBuffer() = default;

  RingBuffer(size_t size, const T& value) : buffer(size, value), size(size) {
  }

  T& operator[](size_t index) {
    assert(index < size);
    size_t realIndex = wrapIndex(head + index);
    return buffer[realIndex];
  }

  const T& operator[](size_t index) const {
    assert(index < size);
    size_t realIndex = wrapIndex(head + index);
    return buffer[realIndex];
  }

  void Scroll(int lines) {
    if (lines >= 0)
      head = wrapIndex(head + lines);
    else
      head = wrapIndex(head + size + lines);
  }

  size_t Size() const {
    return size;
  }

  // void resize(size_t newCapacity) {
  // }

  // begin is the same as end for ring buffer
  Iterator Begin() {
    return Iterator(buffer.data(), size, head);
  }
};
