#include "./ui_process.hpp"
#include "glm/gtx/string_cast.hpp"
#include "session/state.hpp"
#include "utils/logger.hpp"
#include <deque>
#include <utility>
#include <vector>
#include "utils/variant.hpp"
#include <boost/core/typeinfo.hpp>

// parse ------------------------------------------
using namespace event;

static int VariantAsInt(const msgpack::type::variant& v) {
  if (v.is_uint64_t()) return v.as_uint64_t();
  if (v.is_int64_t()) return v.as_int64_t();
  LOG_ERR("VariantAsInt: variant is not convertible to int");
  return 0;
}

static const auto gridTypes = vIndicesSet<
  UiEvent,
  GridResize,
  GridClear,
  GridCursorGoto,
  GridLine,
  GridScroll,
  GridDestroy>();

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

// clang-format off
// i don't like clang format on std::visit(overloaded{})
void ProcessUiEvents(SessionHandle& session) {
  auto& editorState = session->editorState;
  auto& uiEvents = session->uiEvents;

  for (int i = 0; i < uiEvents.numFlushes; i++) {
    assert(!uiEvents.queue.empty());
    auto redrawEvents = std::move(uiEvents.queue.front());
    uiEvents.queue.pop_front();

    // don't need this, since win events are executed last,
    // but just for organization/future refactoring
    std::vector<UiEvent*> gridEvents;
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
          editorState.cursor.ModeInfoSet(e);
        },
        [&](OptionSet& e) {
          auto& opts = editorState.uiOptions;
          if (e.name == "guifont") {
            opts.guifont = e.value.as_string();
          } else if (e.name == "linespace") {
            opts.linespace = VariantAsInt(e.value);
          } else if (e.name == "mousehide") {
            opts.mousehide = e.value.as_bool();
          }
        },
        [&](Chdir& e) {
          LOG("chdir: {}", e.dir);
          editorState.currDir = e.dir;
        },
        [&](ModeChange& e) {
          editorState.cursor.SetMode(e);
        },
        [&](MouseOn&) {
          // LOG("mouse_on");
        },
        [&](MouseOff&) {
          // LOG("mouse_off");
        },
        [&](BusyStart&) {
          LOG_INFO("busy_start");
          editorState.cursor.cursorStop = true;
        },
        [&](BusyStop&) {
          editorState.cursor.cursorStop = false;
        },
        [&](UpdateMenu&) {
          // LOG("update_menu");
        },
        [&](DefaultColorsSet& e) {
          // LOG("default_colors_set");
          editorState.hlManager.DefaultColorsSet(e);
        },
        [&](HlAttrDefine& e) {
          // LOG("hl_attr_define");
          editorState.hlManager.HlAttrDefine(e);
        },
        [&](HlGroupSet& e) {
          // not needed to render grids, but used for rendering
          // own elements with consistent highlighting
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
          // execute grid events before win events
          for (auto *event : gridEvents) {
            std::visit(overloaded{
              [&](GridResize& e) {
                // LOG_INFO("GridResize: {}, {}, {}", e.grid, e.width, e.height);
                editorState.gridManager.Resize(e);
                // default window events not send by nvim
                if (e.grid == 1) {
                  editorState.winManager.Pos({1, {}, 0, 0, e.width, e.height});
                } else {
                  // TODO: find a more robust way to handle grid and win events not syncing up
                  // workaround for nvim only sending GridResize but not WinPos/WinFloatPos when resizing
                  editorState.winManager.SyncResize(e.grid);
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
                // update cursor mask
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
              [&](auto&) {
                std::unreachable();
              }
            }, *event);
          }

          // DONT REMOVE, win events should always execute last!
          for (auto *event : winEvents) {
            std::visit(overloaded{
              [&](WinPos& e) {
                // TODO: find a more robust way to handle
                // grid and win events not syncing up

                // LOG_INFO("WinPos {} {} {}", e.grid, e.width, e.height);
                // if no corresponding GridResize was sent, defer event
                // auto it = editorState.gridManager.grids.find(e.grid);
                // if (it == editorState.gridManager.grids.end()) {
                //   LOG_WARN("WinPos: grid {} not found", e.grid);
                //   // uiEvents.queue[0].emplace_front(e);
                //   return;
                // }

                editorState.winManager.Pos(e);

                // // defer event to next flush if size mismatch
                // auto& grid = it->second;
                // if (grid.width == e.width && grid.height == e.height) {
                //   editorState.winManager.Pos(e);
                // } else {
                //   LOG_INFO("deferred WinPos {} {} {} {} {}",
                //     e.grid, grid.width, grid.height, e.width, e.height);
                //   uiEvents.queue[0].emplace_front(e);
                // }
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
                std::unreachable();
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
          if (gridTypes.contains(e->index())) {
            gridEvents.push_back(e);
            return;
          }

          if (winTypes.contains(e->index())) {
            winEvents.push_back(e);
            return;
          }

          // all variant types are handled
          std::unreachable();
        }
      }, event);
    }
  }
}
// clang-format on
