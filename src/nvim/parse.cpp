#include "parse.hpp"

#include "utils/logger.hpp"

#include <string>
#include <string_view>
#include <unordered_map>

static void ParseRedraw(const msgpack::object& params, RedrawState& state);

void ParseRedrawEvents(rpc::Client& client, RedrawState& state) {
  LOG_DISABLE();
  state.numFlushes = 0;
  while (client.HasNotification()) {
    // static int count = 0;
    auto notification = client.PopNotification();
    // LOG("\n\n--------------------------------- {}", count++);
    if (notification.method == "redraw") {
      ParseRedraw(notification.params, state);
    }
  }
  LOG_ENABLE();
}

using UiEventFunc = void (*)(const msgpack::object& args, RedrawState& state);
static std::unordered_map<std::string_view, UiEventFunc> uiEventFuncs = {
  // Global Events ----------------------------------------------------------
  {"set_title", [](const msgpack::object& args, RedrawState& state) {
    state.currEvents().push_back(args.as<SetTitle>());
  }},

  {"set_icon", [](const msgpack::object& args, RedrawState& state) {
    state.currEvents().push_back(args.as<SetIcon>());
  }},

  {"mode_info_set", [](const msgpack::object& args, RedrawState& state) {
    state.currEvents().push_back(args.as<ModeInfoSet>());
  }},

  {"option_set", [](const msgpack::object& args, RedrawState& state) {
    state.currEvents().push_back(args.as<OptionSet>());
  }},

  {"chdir", [](const msgpack::object& args, RedrawState& state) {
    state.currEvents().push_back(args.as<Chdir>());
  }},

  {"mode_change", [](const msgpack::object& args, RedrawState& state) {
    state.currEvents().push_back(args.as<ModeChange>());
  }},

  {"mouse_on", [](const msgpack::object&, RedrawState& state) {
    state.currEvents().push_back(MouseOn{});
  }},

  {"mouse_off", [](const msgpack::object&, RedrawState& state) {
    state.currEvents().push_back(MouseOff{});
  }},

  {"busy_start", [](const msgpack::object&, RedrawState& state) {
    state.currEvents().push_back(BusyStart{});
  }},

  {"busy_stop", [](const msgpack::object&, RedrawState& state) {
    state.currEvents().push_back(BusyStop{});
  }},

  {"update_menu", [](const msgpack::object&, RedrawState& state) {
    state.currEvents().push_back(UpdateMenu{});
  }},

  {"flush", [](const msgpack::object&, RedrawState& state) {
    state.currEvents().push_back(Flush{});
    state.eventsQueue.emplace_back();
    state.numFlushes++;
  }},

  {"default_colors_set", [](const msgpack::object& args, RedrawState& state) {
    state.currEvents().push_back(args.as<DefaultColorsSet>());
  }},

  {"hl_attr_define", [](const msgpack::object& args, RedrawState& state) {
    // LOG("hl_attr_define: {}", ToString(args));
    state.currEvents().push_back(args.as<HlAttrDefine>());
  }},

  {"hl_group_set", [](const msgpack::object& args, RedrawState& state) {
    state.currEvents().push_back(args.as<HlGroupSet>());
  }},

  // Grid Events --------------------------------------------------------------
  {"grid_resize", [](const msgpack::object& args, RedrawState& state) {
    // LOG("grid_resize: {}", ToString(args));
    state.currEvents().push_back(args.as<GridResize>());
  }},

  {"grid_clear", [](const msgpack::object& args, RedrawState& state) {
    state.currEvents().push_back(args.as<GridClear>());
  }},

  {"grid_cursor_goto", [](const msgpack::object& args, RedrawState& state) {
    state.currEvents().push_back(args.as<GridCursorGoto>());
  }},

  {"grid_line", [](const msgpack::object& args, RedrawState& state) {
    // LOG("grid_line: {}", ToString(args));
    auto [grid, row, col_start, cells] =
      args.as<std::tuple<int, int, int, msgpack::object>>();
    GridLine gridLine{grid, row, col_start, {}};

    std::span<const msgpack::object> cellsList(cells.via.array);
    gridLine.cells.reserve(cellsList.size());

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

    state.currEvents().push_back(std::move(gridLine));
  }},

  {"grid_scroll", [](const msgpack::object& args, RedrawState& state) {
    state.currEvents().push_back(args.as<GridScroll>());
  }},

  {"grid_destroy", [](const msgpack::object& args, RedrawState& state) {
    state.currEvents().push_back(args.as<GridDestroy>());
  }},

  // Multigrid Events ------------------------------------------------------------
  {"win_pos", [](const msgpack::object& args, RedrawState& state) {
    auto [grid, win, start_row, start_col, width, height] =
      args.as<std::tuple<int, msgpack::object, int, int, int, int>>();  
    LOG("win_pos: {} {} {} {} {} {}", grid, ToString(win), start_row, start_col, width, height);
  }},

  {"win_float_pos", [](const msgpack::object& args, RedrawState& state) {
    LOG("win_float_pos: {}", ToString(args));
    // auto [grid, win, anchor, anchor_grid, anchor_row, anchor_col, focusable]
    //   = args.as<std::tuple<int, msgpack::object, std::string, int, float, float, bool>>();
    // LOG("win_float_pos: {} {} {} {} {} {} {}", grid, ToString(win), anchor, anchor_grid, anchor_row, anchor_col, focusable);
  }},

  {"win_external_pos", [](const msgpack::object& args, RedrawState& state) {
    auto [grid, pos] = args.as<std::tuple<int, msgpack::object>>();
    LOG("win_external_pos: {} {}", grid, ToString(pos));
  }},

  {"win_hide", [](const msgpack::object& args, RedrawState& state) {
    auto [grid] = args.as<std::tuple<int>>();
    LOG("win_hide: {}", grid);
  }},

  {"win_close", [](const msgpack::object& args, RedrawState& state) {
    auto [grid] = args.as<std::tuple<int>>();
    LOG("win_close: {}", grid);
  }},

  {"msg_set_pos", [](const msgpack::object& args, RedrawState& state) {
    auto [grid, row, scrolled, sep_char] = args.as<std::tuple<int, int, bool, std::string>>();
    LOG("msg_set_pos: {} {} {} {}", grid, row, scrolled, sep_char);
  }},

  {"win_viewport", [](const msgpack::object& args, RedrawState& state) {
    LOG("win_viewport: {}", ToString(args));
    // auto [grid, win, topline, botline, curline, curcol, line_count, scroll_delta] =
    //   args.as<std::tuple<int, msgpack::object, int, int, int, int, int, int>>();
    // LOG("win_viewport: {} {} {} {} {} {} {} {}", grid, ToString(win), topline, botline, curline, curcol, line_count, scroll_delta);
  }},

  {"win_extmark", [](const msgpack::object& args, RedrawState& state) {
    auto [grid, win, ns_id, mark_id, row, col]
      = args.as<std::tuple<int, msgpack::object, int, int, int, int>>();
    LOG("win_extmark: {} {} {} {} {} {}", grid, ToString(win), ns_id, mark_id, row, col);
  }},
};

static void ParseRedraw(const msgpack::object& params, RedrawState &state) {
  std::span<const msgpack::object> paramList(params.via.array);

  for (const auto& param : paramList) {
    auto paramArr = param.via.array;
    std::string_view eventName(paramArr.ptr[0].convert());
    std::span<const msgpack::object> eventArgs(&paramArr.ptr[1], paramArr.size - 1);

    // std::cout << eventName << std::endl;
    auto uiEventFunc = uiEventFuncs[eventName];
    if (uiEventFunc == nullptr) {
      LOG("Unknown event: {}", eventName);
      throw std::runtime_error("Unknown event: " + std::string(eventName));
      continue;
    }
    for (const auto& arg : eventArgs) {
      uiEventFunc(arg, state);
    }
  }
}
