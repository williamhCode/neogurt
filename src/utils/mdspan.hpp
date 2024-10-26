#pragma once
#include <mdspan>

template <typename T>
using vec2 = std::array<typename T::index_type, 2>;

// poor man's submdspan impl
template <typename View>
  requires(View::extents_type::rank() == 2)
auto SubMdspan2d(View span, vec2<View> offset, vec2<View> extents) {
  std::extents shape(extents[0], extents[1]);
  return std::mdspan(
    &span[offset[0], offset[1]],
    std::layout_stride::mapping(shape, span.mapping().strides())
  );
}
