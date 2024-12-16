#include "./ui.hpp"
#include "utils/logger.hpp"
#include <unordered_map>
#include <span>

using namespace event;

// clang-format off
using UiEventFunc = void (*)(const msgpack::object& args, UiEvents& uiEvents);
static const std::unordered_map<std::string_view, UiEventFunc> uiEventFuncs = {
  // Global Events ----------------------------------------------------------
  {"set_title", [](const msgpack::object& args, UiEvents& uiEvents) {
    uiEvents.Curr().emplace_back(args.as<SetTitle>());
  }},

  {"set_icon", [](const msgpack::object& args, UiEvents& uiEvents) {
    uiEvents.Curr().emplace_back(args.as<SetIcon>());
  }},

  {"mode_info_set", [](const msgpack::object& args, UiEvents& uiEvents) {
    uiEvents.Curr().emplace_back(args.as<ModeInfoSet>());
  }},

  {"option_set", [](const msgpack::object& args, UiEvents& uiEvents) {
    // LOG_INFO("option_set: {}", ToString(args));
    uiEvents.Curr().emplace_back(args.as<OptionSet>());
  }},

  {"chdir", [](const msgpack::object& args, UiEvents& uiEvents) {
    uiEvents.Curr().emplace_back(args.as<Chdir>());
  }},

  {"mode_change", [](const msgpack::object& args, UiEvents& uiEvents) {
    uiEvents.Curr().emplace_back(args.as<ModeChange>());
  }},

  {"mouse_on", [](const msgpack::object&, UiEvents& uiEvents) {
    uiEvents.Curr().emplace_back(MouseOn{});
  }},

  {"mouse_off", [](const msgpack::object&, UiEvents& uiEvents) {
    uiEvents.Curr().emplace_back(MouseOff{});
  }},

  {"busy_start", [](const msgpack::object&, UiEvents& uiEvents) {
    uiEvents.Curr().emplace_back(BusyStart{});
  }},

  {"busy_stop", [](const msgpack::object&, UiEvents& uiEvents) {
    uiEvents.Curr().emplace_back(BusyStop{});
  }},

  {"update_menu", [](const msgpack::object&, UiEvents& uiEvents) {
    uiEvents.Curr().emplace_back(UpdateMenu{});
  }},

  {"flush", [](const msgpack::object&, UiEvents& uiEvents) {
    static int i = 0;
    LOG("flush {} ---------------------------- ", i++);
    uiEvents.Curr().emplace_back(Flush{});
    uiEvents.queue.emplace_back();
    uiEvents.numFlushes++;
  }},

  {"default_colors_set", [](const msgpack::object& args, UiEvents& uiEvents) {
    // LOG_INFO("default_colors_set: {}", ToString(args));
    uiEvents.Curr().emplace_back(args.as<DefaultColorsSet>());
  }},

  {"hl_attr_define", [](const msgpack::object& args, UiEvents& uiEvents) {
    // LOG_INFO("hl_attr_define: {}", ToString(args));
    uiEvents.Curr().emplace_back(args.as<HlAttrDefine>());
  }},

  {"hl_group_set", [](const msgpack::object& args, UiEvents& uiEvents) {
    uiEvents.Curr().emplace_back(args.as<HlGroupSet>());
  }},

  // Grid Events --------------------------------------------------------------
  {"grid_resize", [](const msgpack::object& args, UiEvents& uiEvents) {
    LOG("grid_resize: {}", ToString(args));
    uiEvents.Curr().emplace_back(args.as<GridResize>());
  }},

  {"grid_clear", [](const msgpack::object& args, UiEvents& uiEvents) {
    LOG("grid_clear: {}", ToString(args));
    uiEvents.Curr().emplace_back(args.as<GridClear>());
  }},

  {"grid_cursor_goto", [](const msgpack::object& args, UiEvents& uiEvents) {
    uiEvents.Curr().emplace_back(args.as<GridCursorGoto>());
  }},

  {"grid_line", [](const msgpack::object& args, UiEvents& uiEvents) {
    // LOG("grid_line: {}", ToString(args));
    auto [grid, row, col_start, cells, wrap] =
      args.as<std::tuple<int, int, int, msgpack::object, bool>>();
    GridLine gridLine{grid, row, col_start, {}};

    std::span<const msgpack::object> cellsList = cells.via.array;
    gridLine.cells.reserve(cellsList.size());

    int recent_hl_id;
    for (const auto& cell : cellsList) {
      switch (cell.via.array.size) {
        case 3: {
          auto [text, hl_id, repeat] = cell.as<std::tuple<std::string, int, int>>();
          gridLine.cells.emplace_back(std::move(text), hl_id, repeat);
          recent_hl_id = hl_id;
          break;
        }
        case 2: {
          auto [text, hl_id] = cell.as<std::tuple<std::string, int>>();
          gridLine.cells.emplace_back(std::move(text), hl_id);
          recent_hl_id = hl_id;
          break;
        }
        case 1: {
          auto [text] = cell.as<std::tuple<std::string>>();
          gridLine.cells.emplace_back(std::move(text), recent_hl_id);
          break;
        }
      }
    }

    uiEvents.Curr().emplace_back(std::move(gridLine));
  }},

  {"grid_scroll", [](const msgpack::object& args, UiEvents& uiEvents) {
    uiEvents.Curr().emplace_back(args.as<GridScroll>());
  }},

  {"grid_destroy", [](const msgpack::object& args, UiEvents& uiEvents) {
    LOG("grid_destroy: {}", ToString(args));
    uiEvents.Curr().emplace_back(args.as<GridDestroy>());
  }},

  // Multigrid Events ------------------------------------------------------------
  {"win_pos", [](const msgpack::object& args, UiEvents& uiEvents) {
    LOG("win_pos: {}", ToString(args));
    uiEvents.Curr().emplace_back(args.as<WinPos>());
  }},

  {"win_float_pos", [](const msgpack::object& args, UiEvents& uiEvents) {
    LOG("win_float_pos: {}", ToString(args));
    uiEvents.Curr().emplace_back(args.as<WinFloatPos>());
  }},

  {"win_external_pos", [](const msgpack::object& args, UiEvents& uiEvents) {
    // LOG("win_external_pos: {}", ToString(args));
    uiEvents.Curr().emplace_back(args.as<WinExternalPos>());
  }},

  {"win_hide", [](const msgpack::object& args, UiEvents& uiEvents) {
    LOG("win_hide: {}", ToString(args));
    uiEvents.Curr().emplace_back(args.as<WinHide>());
  }},

  {"win_close", [](const msgpack::object& args, UiEvents& uiEvents) {
    LOG("win_close: {}", ToString(args));
    uiEvents.Curr().emplace_back(args.as<WinClose>());
  }},

  {"msg_set_pos", [](const msgpack::object& args, UiEvents& uiEvents) {
    LOG("msg_set_pos: {}", ToString(args));
    uiEvents.Curr().emplace_back(args.as<MsgSetPos>());
  }},

  {"win_viewport", [](const msgpack::object& args, UiEvents& uiEvents) {
    // LOG("win_viewport: {}", ToString(args));
    // LOG_INFO("win_viewport: {}", ToString(args));
    uiEvents.Curr().emplace_back(args.as<WinViewport>());
  }},

  {"win_viewport_margins", [](const msgpack::object& args, UiEvents& uiEvents) {
    // LOG("win_viewport_margins: {}", ToString(args));
    // LOG_INFO("win_viewport_margins: {}", ToString(args));
    uiEvents.Curr().emplace_back(args.as<WinViewportMargins>());
  }},

  {"win_extmark", [](const msgpack::object& args, UiEvents& uiEvents) {
    // LOG("win_extmark: {}", ToString(args));
    uiEvents.Curr().emplace_back(args.as<WinExtmark>());
  }},
};
// clang-format on

static void ParseUiEvents(const msgpack::object& params, UiEvents& uiEvents) {
  std::span<const msgpack::object> events = params.via.array;

  for (const msgpack::object& eventObj : events) {
    std::span<const msgpack::object> args = eventObj.via.array;
    std::string_view eventName = args[0].convert();
    auto eventArgs = args.subspan(1);

    auto it = uiEventFuncs.find(eventName);
    if (it == uiEventFuncs.end()) {
      LOG_WARN("Unknown event: {}", eventName);
      continue;
    }
    auto uiEventFunc = it->second;

    for (const msgpack::object& arg : eventArgs) {
      uiEventFunc(arg, uiEvents);
    }
  }
}

int ParseUiEvents(rpc::Client& client, UiEvents& uiEvents) {
  uiEvents.numFlushes = 0;

  while (client.HasNotification()) {
    auto notification = client.PopNotification();

    if (notification.method == "redraw") {
      ParseUiEvents(notification.params, uiEvents);
    }
  }

  return uiEvents.numFlushes;
}
