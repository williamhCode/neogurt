#include "state.hpp"
#include "glm/gtx/string_cast.hpp"
#include "utils/color.hpp"
#include "utils/logger.hpp"
#include <deque>
#include <vector>
#include "utils/variant.hpp"

void EditorState::Init(const SizeHandler& sizes) {
  winManager.sizes = &sizes;
  winManager.gridManager = &gridManager;
  cursor.Init(sizes.charSize, sizes.dpiScale);
}

// parse ------------------------------------------
using namespace event;

static int VariantAsInt(const msgpack::type::variant& v) {
  if (v.is_uint64_t()) return v.as_uint64_t();
  if (v.is_int64_t()) return v.as_int64_t();
  LOG_ERR("VariantAsInt: variant is not convertible to int");
  return 0;
}

// static const auto gridTypes = vIndicesSet<
//   UiEvent,
//   GridResize,
//   GridClear,
//   GridCursorGoto,
//   GridLine,
//   GridScroll,
//   GridDestroy>();

// doesn't include MsgSetPos, and WinViewportMargins, cuz they should run after
// all the other win events
static const auto winTypes = vIndicesSet<
  UiEvent,
  WinPos,
  WinFloatPos,
  WinExternalPos,
  WinHide,
  WinClose,
  WinViewport,
  WinExtmark>();

#define INDEX_OF(type) vIndex<UiEvent, type>()

// clang-format off
// i hate clang format on std::visit(overloaded{})
bool ParseEditorState(UiEvents& uiEvents, EditorState& editorState) {
  bool processedEvents = uiEvents.numFlushes > 0;
  for (int i = 0; i < uiEvents.numFlushes; i++) {
    auto redrawEvents = std::move(uiEvents.queue.at(0));
    uiEvents.queue.pop_front();

    // occasionally win events are sent before grid events
    // so just handle manually
    std::vector<UiEvent*> winEvents;
    // neovim sends these events before appropriate window is created
    std::vector<WinViewportMargins*> margins;
    std::vector<MsgSetPos*> msgSetPos;

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
            auto& modeInfo = editorState.cursorModes.emplace_back();
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
          editorState.cursor.SetMode(&editorState.cursorModes[e.modeIdx]);
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
              LOG_WARN("unknown hl attr key: {}, type: {}", key, value.type().name());
            }
          }
        },
        [&](HlGroupSet& e) {
          // not needed to render grids, but used for rendering
          // own elements with consistent highlighting
          // editorState.hlGroupTable.emplace(e.id, e.name);
        },
        [&](GridResize& e) {
          editorState.gridManager.Resize(e);
          // default window events not send by nvim
          if (e.grid == 1) {
            editorState.winManager.Pos({1, {}, 0, 0, e.width, e.height});
          }
        },
        [&](GridClear& e) {
          editorState.gridManager.Clear(e);
        },
        [&](GridCursorGoto& e) {
          editorState.cursor.Goto(e);
        },
        [&](GridLine& e) {
          editorState.gridManager.Line(e);
          if (e.grid == editorState.cursor.grid) {
            editorState.cursor.dirty = true;
          }
        },
        [&](GridScroll& e) {
          editorState.gridManager.Scroll(e);
        },
        [&](GridDestroy& e) {
          editorState.gridManager.Destroy(e);
          // TODO: file bug report, win_close not called after tabclose
          // temp fix for bug
          editorState.winManager.Close({e.grid});
        },
        [&](MsgSetPos& e) {
          LOG("MsgSetPos: {}", e.grid);
          msgSetPos.push_back(&e);
        },
        [&](WinViewportMargins& e) {
          LOG("WinViewportMargins: {}", e.grid);
          margins.push_back(&e);
        },
        [&](Flush&) {
          if (redrawEvents.size() <= 1 && uiEvents.numFlushes == 1) {
            processedEvents = false;
          }

          // DONT REMOVE, win events should always execute last!
          for (auto *event : winEvents) {
            std::visit(overloaded{
              [&](WinPos& e) {
                editorState.winManager.Pos(e);
              },
              [&](WinFloatPos& e) {
                editorState.winManager.FloatPos(e);
              },
              [&](WinExternalPos&) {
              },
              [&](WinHide& e) {
                editorState.winManager.Hide(e);
              },
              [&](WinClose& e) {
                editorState.winManager.Close(e);
              },
              [&](WinViewport& e) {
                editorState.winManager.Viewport(e);
              },
              [&](WinExtmark&) {
              },
              [&](auto&) {
                LOG_WARN("unknown event");
              }
            }, *event);
          }
          // apply margins and msg_set_pos after all events
          for (auto* e : margins) {
            editorState.winManager.ViewportMargins(*e);
          }
          for (auto* e : msgSetPos) {
            editorState.winManager.MsgSetPos(*e);
          }
        },
        [&](auto& _e) {
          auto* e = (UiEvent*)&_e;
          if (winTypes.contains(e->index())) {
            winEvents.push_back(e);
            return;
          }

          LOG_WARN("unknown event");
        }
      }, event);
    }
  }

  return processedEvents;
}
// clang-format on
