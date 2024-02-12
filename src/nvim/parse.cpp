#include "parse.hpp"

#include "editor/state.hpp"
#include "util/logger.hpp"
#include "util/timer.hpp"

#include <string_view>
#include <unordered_map>

static void ParseRedraw(const msgpack::object& params);

void ParseNotifications(rpc::Client& client) {
  LOG_DISABLE();
  editorState.numFlushes = 0;
  while (client.HasNotification()) {
    // static int count = 0;
    auto notification = client.PopNotification();
    // LOG("\n\n--------------------------------- {}", count++);
    if (notification.method == "redraw") {
      ParseRedraw(notification.params);
    }
  }
  LOG_ENABLE();
}

using UiEventFunc = void (*)(const msgpack::object& args);
static std::unordered_map<std::string_view, UiEventFunc> uiEventFuncs = {
  // Global Events ----------------------------------------------------------
  {"set_title", [](const msgpack::object& args) {
    auto [title] = args.as<std::tuple<std::string>>();
    LOG("set_title: {}", title);
  }},

  {"set_icon", [](const msgpack::object& args) {
    auto [icon] = args.as<std::tuple<std::string>>();
    LOG("set_icon: {}", icon);
  }},

  {"mode_info_set", [](const msgpack::object& args) {
    auto [enabled, mode_info] =
      args.as<std::tuple<bool, msgpack::object>>();
    LOG("mode_info_set: {} {}", enabled, ToString(mode_info));
  }},

  {"option_set", [](const msgpack::object& args) {
    auto [name, value] = args.as<std::tuple<std::string, msgpack::object>>();
    LOG("option_set: {} {}", name, ToString(value));
  }},

  {"mode_change", [](const msgpack::object& args) {
    auto [mode, mode_idx] = args.as<std::tuple<std::string, int>>();
    LOG("mode_change: {} {}", mode, mode_idx);
  }},

  {"mouse_on", [](const msgpack::object&) {
    LOG("mouse_on");
  }},

  {"mouse_off", [](const msgpack::object&) {
    LOG("mouse_off");
  }},

  {"busy_start", [](const msgpack::object&) {
    LOG("busy_start");
  }},

  {"busy_stop", [](const msgpack::object&) {
    LOG("busy_stop");
  }},

  {"update_menu", [](const msgpack::object&) {
    LOG("update_menu");
  }},

  {"flush", [](const msgpack::object&) {
    editorState.currEvents().push_back(Flush{});
    editorState.redrawEventsQueue.emplace_back();
    editorState.numFlushes++;
  }},

  {"default_colors_set", [](const msgpack::object& args) {
    LOG("default_colors_set");
  }},

  {"hl_attr_define", [](const msgpack::object& args) {
    LOG("hl_attr_define");
  }},

  {"hl_group_set", [](const msgpack::object& args) {
    LOG("hl_group_set");
  }},

  // Grid Events --------------------------------------------------------------
  {"grid_resize", [](const msgpack::object& args) {
    editorState.currEvents().push_back(args.as<GridResize>());
  }},

  {"grid_clear", [](const msgpack::object& args) {
    editorState.currEvents().push_back(args.as<GridClear>());
  }},

  {"grid_cursor_goto", [](const msgpack::object& args) {
    editorState.currEvents().push_back(args.as<GridCursorGoto>());
  }},

  {"grid_line", [](const msgpack::object& args) {
    auto [grid, row, col_start, cells] =
      args.as<std::tuple<int, int, int, msgpack::object>>();
    GridLine gridLine{grid, row, col_start, {}};

    std::span<const msgpack::object> cellsList(cells.via.array);
    int curr_hl_id = -1;
    for (const auto& cell : cellsList) {
      switch (cell.via.array.size) {
        case 3: {
          auto [text, hl_id, repeat] = cell.as<std::tuple<std::string, int, int>>();
          curr_hl_id = hl_id;
          gridLine.cells.push_back({text, hl_id, repeat});
          break;
        }
        case 2: {
          auto [text, hl_id] = cell.as<std::tuple<std::string, int>>();
          curr_hl_id = hl_id;
          gridLine.cells.push_back({text, hl_id});
          break;
        }
        case 1: {
          auto [text] = cell.as<std::tuple<std::string>>();
          gridLine.cells.push_back({text, curr_hl_id});
          break;
        }
      }
    }

    editorState.currEvents().push_back(std::move(gridLine));
  }},

  {"grid_scroll", [](const msgpack::object& args) {
    editorState.currEvents().push_back(args.as<GridScroll>());
  }},

  {"grid_destroy", [](const msgpack::object& args) {
    editorState.currEvents().push_back(args.as<GridDestroy>());
  }},

  // Multigrid Events ------------------------------------------------------------
  {"win_pos", [](const msgpack::object& args) {
    auto [grid, win, start_row, start_col, width, height] =
      args.as<std::tuple<int, msgpack::object, int, int, int, int>>();  
    LOG("win_pos: {} {} {} {} {} {}", grid, ToString(win), start_row, start_col, width, height);
  }},

  {"win_float_pos", [](const msgpack::object& args) {
    auto [grid, win, anchor, anchor_grid, anchor_row, anchor_col, focusable]
      = args.as<std::tuple<int, msgpack::object, std::string, int, float, float, bool>>();
    LOG("win_float_pos: {} {} {} {} {} {} {}", grid, ToString(win), anchor, anchor_grid, anchor_row, anchor_col, focusable);
  }},

  {"win_external_pos", [](const msgpack::object& args) {
    auto [grid, pos] = args.as<std::tuple<int, msgpack::object>>();
    LOG("win_external_pos: {} {}", grid, ToString(pos));
  }},

  {"win_hide", [](const msgpack::object& args) {
    auto [grid] = args.as<std::tuple<int>>();
    LOG("win_hide: {}", grid);
  }},

  {"win_close", [](const msgpack::object& args) {
    auto [grid] = args.as<std::tuple<int>>();
    LOG("win_close: {}", grid);
  }},

  {"msg_set_pos", [](const msgpack::object& args) {
    auto [grid, row, scrolled, sep_char] = args.as<std::tuple<int, int, bool, std::string>>();
    LOG("msg_set_pos: {} {} {} {}", grid, row, scrolled, sep_char);
  }},

  {"win_viewport", [](const msgpack::object& args) {
    auto [grid, win, topline, botline, curline, curcol, line_count, scroll_delta] =
      args.as<std::tuple<int, msgpack::object, int, int, int, int, int, int>>();
    LOG("win_viewport: {} {} {} {} {} {} {} {}", grid, ToString(win), topline, botline, curline, curcol, line_count, scroll_delta);
  }},

  {"win_extmark", [](const msgpack::object& args) {
    auto [grid, win, ns_id, mark_id, row, col]
      = args.as<std::tuple<int, msgpack::object, int, int, int, int>>();
    LOG("win_extmark: {} {} {} {} {} {}", grid, ToString(win), ns_id, mark_id, row, col);
  }},
};

static void ParseRedraw(const msgpack::object& params) {
  std::span<const msgpack::object> paramList(params.via.array);

  for (const auto& param : paramList) {
    auto paramArr = param.via.array;
    std::string_view eventName(paramArr.ptr[0].convert());
    std::span<const msgpack::object> eventArgs(&paramArr.ptr[1], paramArr.size - 1);

    auto uiEventFunc = uiEventFuncs[eventName];
    if (uiEventFunc == nullptr) {
      LOG("Unknown event: {}", eventName);
      continue;
    }
    for (const auto& arg : eventArgs) {
      uiEventFunc(arg);
    }
  }
}
