#pragma once

#include "editor/grid.hpp"
#include "nvim/msgpack_rpc/client.hpp"
#include <functional>
#include <string_view>
#include <unordered_map>
#include <iostream>

static void ParseRedraw(msgpack::object_handle& params);

inline void ParseNotifications(rpc::Client& client) {
  while (client.HasNotification()) {
    static int count = 0;
    auto notification = client.PopNotification();
    std::cout << "\n\n--------------------------------- " << count << std::endl;
    // std::cout << "method: " << notification.method << std::endl;
    // std::cout << "params: " << notification.params.get() << std::endl;
    count++;
    if (notification.method == "redraw") {
      ParseRedraw(notification.params);
    }
  }
}

using UiEventFunc = void (*)(const msgpack::object& args);
static std::unordered_map<std::string_view, UiEventFunc> uiEventFuncs = {
  {"flush", [](const msgpack::object& args) { std::cout << "flush" << std::endl; }},

  {"default_colors_set", [](const msgpack::object& args) {

  }},

  // Grid Events ------------------------------------------------------------------
  {"grid_resize", [](const msgpack::object& args) {
    auto [grid, width, height] = args.as<std::tuple<int, int, int>>();
    std::cout << "grid_resize: " << grid << " " << width << " " << height << std::endl;
  }},

  {"grid_clear", [](const msgpack::object& args) {
    auto [grid] = args.as<std::tuple<int>>();
    std::cout << "grid_clear: " << grid << std::endl;
  }},

  {"grid_cursor_goto", [](const msgpack::object& args) {
    auto [grid, row, col] = args.as<std::tuple<int, int, int>>();
    std::cout << "grid_cursor_goto: " << grid << " " << row << " " << col << std::endl;
  }},

  {"grid_line", [](const msgpack::object& args) {
    auto [grid, row, colStart, cells] =
      args.as<std::tuple<int, int, int, msgpack::object>>();
    std::cout << "grid_line: " << grid << " " << row << " " << colStart << " " << cells
              << std::endl;
  }},

  {"grid_scroll", [](const msgpack::object& args) {
    auto [grid, top, bot, left, right, rows, cols] =
      args.as<std::tuple<int, int, int, int, int, int, int>>();
    std::cout << "grid_scroll: " << grid << " " << top << " " << bot << " " << left
               << " " << right << " " << rows << " " << cols << std::endl;
  }},

  {"grid_destroy", [](const msgpack::object& args) {
    auto [grid] = args.as<std::tuple<int>>();
    std::cout << "grid_destroy: " << grid << std::endl;
  }},

  // Multigrid Events ------------------------------------------------------------
  {"win_pos", [](const msgpack::object& args) {
    auto [grid, win, start_row, start_col, width, height] =
      args.as<std::tuple<int, msgpack::object, int, int, int, int>>();  
    std::cout << "win_pos: " << grid << " " << win << " " << start_row << " " << start_col
              << " " << width << " " << height << std::endl;
  }},

  // {"win_float_pos", [](const msgpack::object& args) {
  //   auto [grid, win, anchor, anchor_grid, anchor_row, anchor_col, focusable]
  // }},

  // {"win_external_pos", [](const msgpack::object& args) {
  //   auto [grid, pos]
  // }},

  // {"win_hide", [](const msgpack::object& args) {
  //   auto [grid] = args.as<std::tuple<int>>();
  //   std::cout << "win_hide: " << win << std::endl;
  // }},

  // {"win_close", [](const msgpack::object& args) {
  //   auto [grid] = args.as<std::tuple<int>>();
  //   std::cout << "win_close: " << win << std::endl;
  // }},

  // {"msg_set_pos", [](const msgpack::object& args) {
  //   auto [grid, row, scrolled, sep_char]
  // }},

  {"win_viewport", [](const msgpack::object& args) {
    auto [grid, win, topline, botline, curline, curcol, line_count, scroll_delta] =
      args.as<std::tuple<int, msgpack::object, int, int, int, int, int, int>>();
    std::cout << "win_viewport: " << grid << " " << win << " " << topline << " " << botline
              << " " << curline << " " << curcol << " " << line_count << " " << scroll_delta
              << std::endl;

    // auto size = win.via.ext.size;
    // auto data = reinterpret_cast<const int8_t*>(win.via.ext.data());
    // int result = 0;
    // result |= static_cast<int>(data[0]) << 16; // Shift the first byte left by 16 bits
    // result |= static_cast<int>(data[1]) << 8;  // Shift the second byte left by 8 bits
    // result |= static_cast<int>(data[2]);
    // std::cout << "id: " << result << std::endl;
  }},

  {"win_extmark", [](const msgpack::object& args) {
    auto [grid, win, ns_id, mark_id, row, col]
      = args.as<std::tuple<int, msgpack::object, int, int, int, int>>();
    std::cout << "win_extmark: " << grid << " " << win << " " << ns_id << " " << mark_id << " "
              << row << " " << col << std::endl;
  }},
};

static void ParseRedraw(msgpack::object_handle& params) {
  std::span<const msgpack::object> paramList(params->via.array);

  for (const auto& param : paramList) {
    auto paramArr = param.via.array;
    std::string_view eventName(paramArr.ptr[0].convert());
    std::span<const msgpack::object> eventArgs(&paramArr.ptr[1], paramArr.size - 1);

    auto uiEventFunc = uiEventFuncs[eventName];
    if (uiEventFunc == nullptr) {
      // std::cout << "Unknown event: " << eventName << std::endl;
      continue;
    }
    for (const auto& arg : eventArgs) {
      uiEventFunc(arg);
    }
  }
}
