#pragma once

#include <variant>

template <class... Ts>
struct overloaded : Ts... {
  using Ts::operator()...;
};

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

template <typename T>
auto& vGet(auto& t) noexcept {
  return *std::get_if<T>(&t);
}

// std::visit([&](auto&& e) {
//   using T = std::decay_t<decltype(e)>;
//   if constexpr (std::is_same_v<T, Flush>) {
//     // LOG("flush");

//   } else if constexpr (std::is_same_v<T, GridResize>) {
//     auto& [grid, width, height] = e;
//     // LOG("grid_resize");

//   } else if constexpr (std::is_same_v<T, GridClear>) {
//     auto& [grid] = e;
//     // LOG("grid_clear");

//   } else if constexpr (std::is_same_v<T, GridCursorGoto>) {
//     auto& [grid, row, col] = e;
//     // LOG("grid_cursor_goto");

//   } else if constexpr (std::is_same_v<T, GridLine>) {
//     auto& [grid, row, colStart, cells] = e;
//     // LOG("grid_line");

//   } else if constexpr (std::is_same_v<T, GridScroll>) {
//     auto& [grid, top, bot, left, right, rows, cols] = e;
//     // LOG("grid_scroll");

//   } else if constexpr (std::is_same_v<T, GridDestroy>) {
//     auto& [grid] = e;
//     // LOG("grid_destroy");
//   }
// }, event);

// switch (event.index()) {
//   case vIndex<RedrawEvent, Flush>(): {
//     // LOG("flush");
//     break;
//   }
//   case vIndex<RedrawEvent, GridResize>(): {
//     auto& [grid, width, height] = vGet<GridResize>(event);
//     // LOG("grid_resize");
//     break;
//   }
//   case vIndex<RedrawEvent, GridClear>(): {
//     auto& [grid] = vGet<GridClear>(event);
//     // LOG("grid_clear");
//     break;
//   }
//   case vIndex<RedrawEvent, GridCursorGoto>(): {
//     auto& [grid, row, col] = vGet<GridCursorGoto>(event);
//     // LOG("grid_cursor_goto");
//     break;
//   }
//   case vIndex<RedrawEvent, GridLine>(): {
//     auto& [grid, row, colStart, cells] = vGet<GridLine>(event);
//     // LOG("grid_line");
//     break;
//   }
//   case vIndex<RedrawEvent, GridScroll>(): {
//     auto& [grid, top, bot, left, right, rows, cols] = vGet<GridScroll>(event);
//     // LOG("grid_scroll");
//     break;
//   }
//   case vIndex<RedrawEvent, GridDestroy>(): {
//     auto& [grid] = vGet<GridDestroy>(event);
//     // LOG("grid_destroy");
//     break;
//   }
// }
