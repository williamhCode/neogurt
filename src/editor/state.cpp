#include "state.hpp"
#include "glm/ext/vector_float4.hpp"
#include "utils/variant.hpp"

auto IntToColor(int color) {
  return glm::vec4(
    ((color >> 16) & 0xff) / 255.0f,
    ((color >> 8) & 0xff) / 255.0f,
    (color & 0xff) / 255.0f,
    1.0f
  );
};

void ProcessRedrawEvents(RedrawState& redrawState, EditorState& editorState) {
  for (int i = 0; i < redrawState.numFlushes; i++) {
    auto& redrawEvents = redrawState.eventsQueue.front();
    for (auto& event : redrawEvents) {
      std::visit(
        overloaded{
          [&](SetTitle& e) {
            // LOG("set_title");
          },
          [&](SetIcon& e) {
            // LOG("set_icon");
          },
          [&](ModeInfoSet& e) {
            // LOG("mode_info_set");
            for (auto& elem : e.modeInfo) {
              for (auto& [key, value] : elem) {
                if (value.is_string()) {
                  // LOG("mode_info: {} {}", key, value.as_string());
                } else if (value.is_uint64_t()) {
                  // LOG("mode_info: {} {}", key, value.as_uint64_t());
                } else {
                  assert(false);
                }
              }
            }
          },
          [&](OptionSet& e) {
            // LOG("option_set");
          },
          [&](Chdir& e) {
            // LOG("chdir");
          },
          [&](ModeChange& e) {
            // LOG("mode_change");
          },
          [&](MouseOn&) {
            // LOG("mouse_on");
          },
          [&](MouseOff&) {
            // LOG("mouse_off");
          },
          [&](BusyStart&) {
            // LOG("busy_start");
          },
          [&](BusyStop&) {
            // LOG("busy_stop");
          },
          [&](UpdateMenu&) {
            // LOG("update_menu");
          },
          [&](DefaultColorsSet& e) {
            // LOG("default_colors_set");
            auto& hl = editorState.hlTable[0];
            hl.foreground = IntToColor(e.rgbFg);
            hl.background = IntToColor(e.rgbBg);
            hl.special = IntToColor(e.rgbSp);
          },
          [&](HlAttrDefine& e) {
            // LOG("hl_attr_define");
            auto& hl = editorState.hlTable[e.id];
            for (auto& [key, value] : e.rgbAttrs) {
              if (key == "foreground") {
                hl.foreground = IntToColor(value.as_uint64_t());
              } else if (key == "background") {
                hl.background = IntToColor(value.as_uint64_t());
              } else if (key == "special") {
                hl.special = IntToColor(value.as_uint64_t());
              } else if (key == "reverse") {
                hl.reverse = value.as_bool();
              } else if (key == "italic") {
                hl.italic = value.as_bool();
              } else if (key == "bold") {
                hl.bold = value.as_bool();
              } else if (key == "strikethrough") {
                hl.strikethrough = value.as_bool();
              } else if (key == "underline") {
                hl.underline = UnderlineType::Underline;
              } else if (key == "undercurl") {
                hl.underline = UnderlineType::Undercurl;
              } else if (key == "underdouble") {
                hl.underline = UnderlineType::Underdouble;
              } else if (key == "underdotted") {
                hl.underline = UnderlineType::Underdotted;
              } else if (key == "underdashed") {
                hl.underline = UnderlineType::Underdashed;
              } else if (key == "blend") {
                hl.blend = value.as_uint64_t();
              } else {
                assert(false);
              }
            }
          },
          [&](HlGroupSet& e) {
            // LOG("hl_group_set: {} {}", e.name, e.id);
          },
          [&](Flush&) {
          },
          [&](GridResize& e) {
            editorState.gridManager.Resize(e);
          },
          [&](GridClear& e) {
            editorState.gridManager.Clear(e);
          },
          [&](GridCursorGoto& e) {
            editorState.gridManager.CursorGoto(e);
          },
          [&](GridLine& e) {
            editorState.gridManager.Line(e);
          },
          [&](GridScroll& e) {
            editorState.gridManager.Scroll(e);
          },
          [&](GridDestroy& e) {
            editorState.gridManager.Destroy(e);
          },
          [&](auto&) {
            LOG("unknown event");
          },
        },
        event
      );
    }
    redrawState.eventsQueue.pop_front();
  }
}
