#pragma once

#include <variant>
#include <set>

template <class... Ts>
struct overloaded : Ts... {
  using Ts::operator()...;
};
// explicit deduction guide (not needed as of C++20)
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

template <typename VariantType, typename T, std::size_t index = 0>
constexpr std::size_t vIndex() {
  static_assert(std::variant_size_v<VariantType> > index, "Type not found in variant");
  if constexpr (index == std::variant_size_v<VariantType>) {
    return index;
  } else if constexpr (std::is_same_v<
                         std::variant_alternative_t<index, VariantType>, T>) {
    return index;
  } else {
    return vIndex<VariantType, T, index + 1>();
  }
}

template <typename VariantType, typename... Types>
constexpr auto vIndicesSet() {
  return std::set<std::size_t>{vIndex<VariantType, Types>()...};
}

template <typename T>
auto& vGet(auto& t) noexcept {
  return *std::get_if<T>(&t);
}
