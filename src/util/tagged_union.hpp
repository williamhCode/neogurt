#pragma once

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
  struct _Index<T, F, R...> : std::integral_constant<size_t, 1 + _Index<T, R...>::value> {
  };

  static constexpr size_t invalid_index = -1;

  size_t typeIndex = invalid_index;
  std::aligned_union_t<0, Types...> data;

  // Helper to destroy the current data
  void destroy() {
    if (typeIndex != invalid_index) {
      // Call the destructor of the current type
      ((void)reinterpret_cast<Types*>(&data)->~Types(), ...);
      typeIndex = invalid_index;
    }
  }

public:
  TaggedUnion() = default;

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
    new (&data) T(std::forward<T>(value));
    typeIndex = _Index<T, Types...>();
  }

  template <SameAsAny<Types...> T>
  void Set(T&& value) {
    destroy();
    new (&data) T(std::forward<T>(value));
    typeIndex = _Index<T, Types...>();
  }

  template <SameAsAny<Types...> T>
  T& Get() {
    return *reinterpret_cast<T*>(&data);
  }

  template <SameAsAny<Types...> T>
  const T& Get() const {
    return *reinterpret_cast<const T*>(&data);
  }

  size_t Index() const {
    return typeIndex;
  }

  // Static method to get the index of a type
  template <SameAsAny<Types...> T>
  static constexpr size_t Index() {
    return _Index<T, Types...>::value;
  }
};
