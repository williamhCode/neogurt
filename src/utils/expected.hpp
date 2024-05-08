#pragma once
#include <expected>

// constructs a new std::unexpected from an std::expected::error()
// use only when propogating (returning) unexpected value into
// std::expected with different value type
template<typename T, typename E>
constexpr auto MakeUnexpected(std::expected<T, E>& expected) noexcept {
  return std::unexpected(std::move(expected.error()));
}
