#include "state.hpp"
#include "app/options.hpp"
#include "glm/gtx/string_cast.hpp"
#include "utils/color.hpp"
#include "utils/variant.hpp"
#include "utils/logger.hpp"
#include <algorithm>
#include <deque>
#include <vector>

static int VariantAsInt(const msgpack::type::variant& v) {
  if (v.is_uint64_t()) return v.as_uint64_t();
  if (v.is_int64_t()) return v.as_int64_t();
  LOG_ERR("VariantAsInt: variant is not convertible to int");
  return 0;
}

// clang-format off
// i hate clang format format on std::visit(overloaded{})
bool ParseEditorState(UiEvents& uiEvents, EditorState& editorState) {
  bool processedEvents = uiEvents.numFlushes > 0;
  for (int i = 0; i < uiEvents.numFlushes; i++) {
    auto& redrawEvents = uiEvents.queue.front();
    // sort based on variant index
    std::ranges::sort(redrawEvents, [](auto& a, auto& b) {
      return a.index() < b.index();
    });

    for (auto& event : redrawEvents) {
      std::visit(overloaded{
        [&](SetTitle& e) {
          // LOG("set_title");
        },
        [&](SetIcon& e) {
          // LOG("set_icon");
        },
        [&](ModeInfoSet& e) {
          for (auto& elem : e.modeInfo) {
            auto& modeInfo = editorState.modeInfoList.emplace_back();
            for (auto& [key, value] : elem) {
              if (key == "cursor_shape") {
                auto shape = value.as_string();
                if (shape == "block") {
                  modeInfo.cursorShape = CursorShape::Block;
                } else if (shape == "horizontal") {
                  modeInfo.cursorShape = CursorShape::Horizontal;
                } else if (shape == "vertical") {
                  modeInfo.cursorShape = CursorShape::Vertical;
                } else {
                  LOG_WARN("unknown cursor shape: {}", shape);
                }
              } else if (key == "cell_percentage") {
                modeInfo.cellPercentage = VariantAsInt(value);
              } else if (key == "blinkwait") {
                modeInfo.blinkwait = VariantAsInt(value);
              } else if (key == "blinkon") {
                modeInfo.blinkon = VariantAsInt(value);
              } else if (key == "blinkoff") {
                modeInfo.blinkoff = VariantAsInt(value);
              } else if (key == "attr_id") {
                modeInfo.attrId = VariantAsInt(value);
              }
            }
          }
        },
        [&](OptionSet& e) {
          auto& opts = editorState.uiOptions;
          if (e.name == "emoji") {
            opts.emoji = e.value.as_bool();
          } else if (e.name == "guifont") {
            opts.guifont = e.value.as_string();
          } else if (e.name == "guifontwide") {
            opts.guifontwide = e.value.as_string();
          } else if (e.name == "linespace") {
            opts.linespace = VariantAsInt(e.value);
          } else if (e.name == "mousefocus") {
            opts.mousefocus = e.value.as_bool();
          } else if (e.name == "mousehide") {
            opts.mousehide = e.value.as_bool();
          } else if (e.name == "mousemoveevent") {
            opts.mousemoveevent = e.value.as_bool();
          } else {
            // LOG_WARN("unhandled option_set: {}", e.name);
          }
        },
        [&](Chdir& e) {
          // LOG("chdir");
        },
        [&](ModeChange& e) {
          editorState.cursor.SetMode(&editorState.modeInfoList[e.modeIdx]);
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
          if (!hl.background.has_value()) {
            hl.background = IntToColor(e.rgbBg);
          }
          hl.special = IntToColor(e.rgbSp);

          auto& color = editorState.winManager.clearColor;
          color.r = hl.background->r * 255;
          color.g = hl.background->g * 255;
          color.b = hl.background->b * 255;
          color.a = hl.background->a * 255;
        },
        [&](HlAttrDefine& e) {
          // LOG("hl_attr_define");
          auto& hl = editorState.hlTable[e.id];
          for (auto& [key, value] : e.rgbAttrs) {
            if (key == "foreground") {
              hl.foreground = IntToColor(VariantAsInt(value));
            } else if (key == "background") {
              hl.background = IntToColor(VariantAsInt(value));
            } else if (key == "special") {
              hl.special = IntToColor(VariantAsInt(value));
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
              hl.bgAlpha = 1 - (VariantAsInt(value) / 100.0f);
            } else {
              LOG_WARN("unknown hl attr key: {}", key);
            }
          }
        },
        [&](HlGroupSet& e) {
          // not needed to render grids, but used for rendering
          // own elements with consistent highlighting
          // editorState.hlGroupTable.emplace(e.id, e.name);
        },
        [&](Flush&) {
          if (redrawEvents.size() <= 1 && uiEvents.numFlushes == 1) {
            processedEvents = false;
          }
        },
        [&](GridResize& e) {
          LOG("GridResize: {}", e.grid);
          editorState.gridManager.Resize(e);
          // default window events not send by nvim
          if (e.grid == 1) {
            editorState.winManager.Pos({1, {}, 0, 0, e.width, e.height});
          }
        },
        [&](GridClear& e) {
          LOG("GridClear: {}", e.grid);
          editorState.gridManager.Clear(e);
        },
        [&](GridCursorGoto& e) {
          editorState.gridManager.CursorGoto(e);
          editorState.winManager.activeWinId = e.grid;
        },
        [&](GridLine& e) {
          editorState.gridManager.Line(e);
        },
        [&](GridScroll& e) {
          editorState.gridManager.Scroll(e);
        },
        [&](GridDestroy& e) {
          LOG("GridDestroy: {}", e.grid);
          editorState.gridManager.Destroy(e);
          // TODO: file bug report, win_close not called after tabclose
          // temp fix for bug
          editorState.winManager.Close({e.grid});
        },
        [&](WinPos& e) {
          LOG("WinPos: {}", e.grid);
          editorState.winManager.Pos(e);
        },
        [&](WinFloatPos& e) {
          LOG("WinFloatPos: {}", e.grid);
          editorState.winManager.FloatPos(e);
        },
        [&](WinExternalPos& e) {
        },
        [&](WinHide& e) {
          LOG("WinHide: {}", e.grid);
          editorState.winManager.Hide(e);
        },
        [&](WinClose& e) {
          LOG("WinClose: {}", e.grid);
          editorState.winManager.Close(e);
        },
        [&](MsgSetPos& e) {
          LOG("MsgSetPos: {}", e.grid);
          editorState.winManager.MsgSet(e);
        },
        [&](WinViewport& e) {
          editorState.winManager.Viewport(e);
        },
        [&](WinViewportMargins& e) {
          LOG("WinViewportMargins: {}", e.grid);
          editorState.winManager.ViewportMargins(e);
        },
        [&](WinExtmark& e) {
        },
        [&](auto&) {
          LOG_WARN("unknown event");
        },
      }, event);
    }
    uiEvents.queue.pop_front();
  }

  return processedEvents;
}
// clang-format on
