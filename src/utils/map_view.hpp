#pragma once

#include <cstddef>
#include <iterator>

template <typename T>
class MapView {
private:
  T* data;
  size_t size;

public:
  MapView(T* data, size_t size) : data(data), size(size) {
  }

  class Iterator {
  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = T*;
    using reference = T&;

    Iterator(pointer ptr, size_t index) : ptr(ptr), index(index) {
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
    size_t index;
  };

  Iterator begin() {
    return Iterator(data, 0);
  }

  Iterator end() {
    return Iterator(data, size);
  }

  const Iterator begin() const {
    return Iterator(data, 0);
  }

  const Iterator end() const {
    return Iterator(data, size);
  }
};
