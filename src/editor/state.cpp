#include "state.hpp"
#include "glm/gtx/string_cast.hpp"
#include "nvim/events/parse.hpp"
#include "utils/color.hpp"
#include "utils/logger.hpp"
#include <deque>
#include <vector>
#include "utils/variant.hpp"

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
    //   switch (event.index()) {
    //     case INDEX_OF(SetTitle): {
    //       break;
    //     }
    //     case INDEX_OF(SetIcon): {
    //       break;
    //     }
    //     case INDEX_OF(ModeInfoSet): {
    //       auto& e = std::get<ModeInfoSet>(event);
    //       for (auto& elem : e.modeInfo) {
    //         auto& modeInfo = editorState.modeInfoList.emplace_back();
    //         for (auto& [key, value] : elem) {
    //           if (key == "cursor_shape") {
    //             auto shape = value.as_string();
    //             if (shape == "block") {
    //               modeInfo.cursorShape = CursorShape::Block;
    //             } else if (shape == "horizontal") {
    //               modeInfo.cursorShape = CursorShape::Horizontal;
    //             } else if (shape == "vertical") {
    //               modeInfo.cursorShape = CursorShape::Vertical;
    //             } else {
    //               LOG_WARN("unknown cursor shape: {}", shape);
    //             }
    //           } else if (key == "cell_percentage") {
    //             modeInfo.cellPercentage = VariantAsInt(value);
    //           } else if (key == "blinkwait") {
    //             modeInfo.blinkwait = VariantAsInt(value);
    //           } else if (key == "blinkon") {
    //             modeInfo.blinkon = VariantAsInt(value);
    //           } else if (key == "blinkoff") {
    //             modeInfo.blinkoff = VariantAsInt(value);
    //           } else if (key == "attr_id") {
    //             modeInfo.attrId = VariantAsInt(value);
    //           }
    //         }
    //       }
    //       break;
    //     }
    //     case INDEX_OF(OptionSet): {
    //       auto& e = std::get<OptionSet>(event);
    //       auto& opts = editorState.uiOptions;
    //       if (e.name == "emoji") {
    //         opts.emoji = e.value.as_bool();
    //       } else if (e.name == "guifont") {
    //         opts.guifont = e.value.as_string();
    //       } else if (e.name == "guifontwide") {
    //         opts.guifontwide = e.value.as_string();
    //       } else if (e.name == "linespace") {
    //         opts.linespace = VariantAsInt(e.value);
    //       } else if (e.name == "mousefocus") {
    //         opts.mousefocus = e.value.as_bool();
    //       } else if (e.name == "mousehide") {
    //         opts.mousehide = e.value.as_bool();
    //       } else if (e.name == "mousemoveevent") {
    //         opts.mousemoveevent = e.value.as_bool();
    //       } else {
    //         // LOG_WARN("unhandled option_set: {}", e.name);
    //       }
    //       break;
    //     }
    //     case INDEX_OF(Chdir): {
    //       // LOG("chdir");
    //       break;
    //     }
    //     case INDEX_OF(ModeChange): {
    //       auto& e = std::get<ModeChange>(event);
    //       editorState.cursor.SetMode(&editorState.modeInfoList[e.modeIdx]);
    //       break;
    //     }
    //     case INDEX_OF(MouseOn): {
    //       // LOG("mouse_on");
    //       break;
    //     }
    //     case INDEX_OF(MouseOff): {
    //       // LOG("mouse_off");
    //       break;
    //     }
    //     case INDEX_OF(BusyStart): {
    //       // LOG("busy_start");
    //       break;
    //     }
    //     case INDEX_OF(BusyStop): {
    //       // LOG("busy_stop");
    //       break;
    //     }
    //     case INDEX_OF(UpdateMenu): {
    //       // LOG("update_menu");
    //       break;
    //     }
    //     case INDEX_OF(DefaultColorsSet): {
    //       auto& e = std::get<DefaultColorsSet>(event);
    //       // LOG("default_colors_set");
    //       auto& hl = editorState.hlTable[0];
    //       hl.foreground = IntToColor(e.rgbFg);
    //       if (!hl.background.has_value()) {
    //         hl.background = IntToColor(e.rgbBg);
    //       }
    //       hl.special = IntToColor(e.rgbSp);

    //       auto& color = editorState.winManager.clearColor;
    //       color.r = hl.background->r * 255;
    //       color.g = hl.background->g * 255;
    //       color.b = hl.background->b * 255;
    //       color.a = hl.background->a * 255;
    //       break;
    //     }
    //     case INDEX_OF(HlAttrDefine): {
    //       auto& e = std::get<HlAttrDefine>(event);
    //       // LOG("hl_attr_define");
    //       auto& hl = editorState.hlTable[e.id];
    //       for (auto& [key, value] : e.rgbAttrs) {
    //         if (key == "foreground") {
    //           hl.foreground = IntToColor(VariantAsInt(value));
    //         } else if (key == "background") {
    //           hl.background = IntToColor(VariantAsInt(value));
    //         } else if (key == "special") {
    //           hl.special = IntToColor(VariantAsInt(value));
    //         } else if (key == "reverse") {
    //           hl.reverse = value.as_bool();
    //         } else if (key == "italic") {
    //           hl.italic = value.as_bool();
    //         } else if (key == "bold") {
    //           hl.bold = value.as_bool();
    //         } else if (key == "strikethrough") {
    //           hl.strikethrough = value.as_bool();
    //         } else if (key == "underline") {
    //           hl.underline = UnderlineType::Underline;
    //         } else if (key == "undercurl") {
    //           hl.underline = UnderlineType::Undercurl;
    //         } else if (key == "underdouble") {
    //           hl.underline = UnderlineType::Underdouble;
    //         } else if (key == "underdotted") {
    //           hl.underline = UnderlineType::Underdotted;
    //         } else if (key == "underdashed") {
    //           hl.underline = UnderlineType::Underdashed;
    //         } else if (key == "blend") {
    //           hl.bgAlpha = 1 - (VariantAsInt(value) / 100.0f);
    //         } else {
    //           LOG_WARN("unknown hl attr key: {}", key);
    //         }
    //       }
    //       break;
    //     }
    //     case INDEX_OF(HlGroupSet): {
    //       // not needed to render grids, but used for rendering
    //       // own elements with consistent highlighting
    //       // editorState.hlGroupTable.emplace(e.id, e.name);
    //       break;
    //     }
    //     case INDEX_OF(GridResize): {
    //       auto& e = std::get<GridResize>(event);
    //       editorState.gridManager.Resize(e);
    //       // default window events not send by nvim
    //       if (e.grid == 1) {
    //         editorState.winManager.Pos({1, {}, 0, 0, e.width, e.height});
    //       }
    //       break;
    //     }
    //     case INDEX_OF(GridClear): {
    //       auto& e = std::get<GridClear>(event);
    //       editorState.gridManager.Clear(e);
    //       break;
    //     }
    //     case INDEX_OF(GridCursorGoto): {
    //       auto& e = std::get<GridCursorGoto>(event);
    //       editorState.gridManager.CursorGoto(e);
    //       editorState.winManager.activeWinId = e.grid;
    //       break;
    //     }
    //     case INDEX_OF(GridLine): {
    //       auto& e = std::get<GridLine>(event);
    //       editorState.gridManager.Line(e);
    //       break;
    //     }
    //     case INDEX_OF(GridScroll): {
    //       auto& e = std::get<GridScroll>(event);
    //       editorState.gridManager.Scroll(e);
    //       break;
    //     }
    //     case INDEX_OF(GridDestroy): {
    //       auto& e = std::get<GridDestroy>(event);
    //       editorState.gridManager.Destroy(e);
    //       // TODO: file bug report, win_close not called after tabclose
    //       // temp fix for bug
    //       editorState.winManager.Close({e.grid});
    //       break;
    //     }
    //     case INDEX_OF(MsgSetPos): {
    //       auto& e = std::get<MsgSetPos>(event);
    //       LOG("MsgSetPos: {}", e.grid);
    //       msgSetPos.push_back(&e);
    //       break;
    //     }
    //     case INDEX_OF(WinViewportMargins): {
    //       auto& e = std::get<WinViewportMargins>(event);
    //       LOG("WinViewportMargins: {}", e.grid);
    //       margins.push_back(&e);
    //       break;
    //     }
    //     case INDEX_OF(Flush): {
    //       if (redrawEvents.size() <= 1 && uiEvents.numFlushes == 1) {
    //         processedEvents = false;
    //       }

    //       // DONT REMOVE, win events should always execute last!
    //       for (auto *eventPtr : winEvents) {
    //         auto& event = *eventPtr;
    //         switch (event.index()) {
    //           case INDEX_OF(WinPos): {
    //             auto& e = std::get<WinPos>(event);
    //             editorState.winManager.Pos(e);
    //             break;
    //           }
    //           case INDEX_OF(WinFloatPos): {
    //             auto& e = std::get<WinFloatPos>(event);
    //             editorState.winManager.FloatPos(e);
    //             break;
    //           }
    //           case INDEX_OF(WinExternalPos): {
    //             break;
    //           }
    //           case INDEX_OF(WinHide): {
    //             auto& e = std::get<WinHide>(event);
    //             editorState.winManager.Hide(e);
    //             break;
    //           }
    //           case INDEX_OF(WinClose): {
    //             auto& e = std::get<WinClose>(event);
    //             editorState.winManager.Close(e);
    //             break;
    //           }
    //           case INDEX_OF(WinViewport): {
    //             auto& e = std::get<WinViewport>(event);
    //             editorState.winManager.Viewport(e);
    //             break;
    //           }
    //           case INDEX_OF(WinExtmark): {
    //             break;
    //           }
    //           default: {
    //             LOG_WARN("unknown event");
    //             break;
    //           }
    //         };
    //       }
    //       // apply margins and msg_set_pos after all events
    //       for (auto* e : margins) {
    //         editorState.winManager.ViewportMargins(*e);
    //       }
    //       for (auto* e : msgSetPos) {
    //         editorState.winManager.MsgSet(*e);
    //       }
    //       break;
    //     }

    //     case INDEX_OF(WinPos):
    //     case INDEX_OF(WinFloatPos):
    //     case INDEX_OF(WinExternalPos):
    //     case INDEX_OF(WinHide):
    //     case INDEX_OF(WinClose):
    //     case INDEX_OF(WinViewport):
    //     case INDEX_OF(WinExtmark) : {
    //       winEvents.push_back(&event);
    //       break;
    //     }

    //     default : {
    //       LOG_WARN("unknown event");
    //       break;
    //     }
    //   }
    // }

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
            editorState.winManager.MsgSet(*e);
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
