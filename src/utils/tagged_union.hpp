#pragma once

#include "utils/logger.hpp"
#include <type_traits>
#include <utility>

template <typename T, typename... Ts>
concept SameAsAny = (... or std::same_as<T, Ts>);

// TaggedUnion class
template <typename... Types>
class TaggedUnion {
private:
  template <typename...>
  struct _Index;

  // found it
  template <typename T, typename... R>
  struct _Index<T, T, R...> : std::integral_constant<size_t, 0> {};

  // still looking
  template <typename T, typename F, typename... R>
  struct _Index<T, F, R...>
      : std::integral_constant<size_t, 1 + _Index<T, R...>::value> {};

  static constexpr size_t invalid_index = -1;

  size_t typeIndex = invalid_index;
  std::aligned_union_t<0, Types...> data;

  // Helper to invoke destructor based on runtime index
  template <size_t I = 0>
  void destroy_helper() {
    if (I == typeIndex) {
      using CurrentType = typename std::tuple_element<I, std::tuple<Types...>>::type;
      reinterpret_cast<CurrentType*>(&data)->~CurrentType();
    } else if constexpr (I + 1 < sizeof...(Types)) {
      destroy_helper<I + 1>();
    }
  }

  void destroy() {
    if (typeIndex != invalid_index) {
      destroy_helper();
      typeIndex = invalid_index;
    }
  }

public:
  TaggedUnion() = default;

  TaggedUnion(const TaggedUnion&) = delete;
  TaggedUnion& operator=(const TaggedUnion&) = delete;

  TaggedUnion(TaggedUnion&& other) noexcept : typeIndex(other.typeIndex) {
    if (typeIndex != invalid_index) {
      std::memcpy(&data, &other.data, sizeof(data));
      other.typeIndex = invalid_index;
    }
  }

  TaggedUnion& operator=(TaggedUnion&& other) noexcept {
    if (this != &other) {
      destroy();
      typeIndex = other.typeIndex;
      if (typeIndex != invalid_index) {
        std::memcpy(&data, &other.data, sizeof(data));
        other.typeIndex = invalid_index;
      }
    }
    return *this;
  }

  template <SameAsAny<Types...> T>
  TaggedUnion(const T& value) {
    Set(value);
  }

  template <SameAsAny<Types...> T>
  TaggedUnion(T&& value) {
    Set(std::move(value));
  }

  ~TaggedUnion() {
    destroy();
  }

  template <SameAsAny<Types...> T>
  void Set(const T& value) {
    destroy();
    // copy data
    new (&data) T(value);
    typeIndex = Type<T>();
  }

  template <SameAsAny<Types...> T>
  void Set(T&& value) {
    destroy();
    new (&data) T(std::forward<T>(value));
    typeIndex = Type<T>();
  }

  template <SameAsAny<Types...> T>
  T& Get() {
    return *reinterpret_cast<T*>(&data);
  }

  template <SameAsAny<Types...> T>
  const T& Get() const {
    return *reinterpret_cast<const T*>(&data);
  }

  size_t Type() const {
    return typeIndex;
  }

  // Static method to get the index of a type
  template <SameAsAny<Types...> T>
  static constexpr size_t Type() {
    return _Index<T, Types...>();
  }
};
