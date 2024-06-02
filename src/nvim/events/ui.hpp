#include "msgpack.hpp"
#include "utils/variant.hpp"
#include <map>

struct SetTitle {
  std::string title;
  MSGPACK_DEFINE(title);
};
struct SetIcon {
  std::string icon;
  MSGPACK_DEFINE(icon);
};
struct ModeInfoSet {
  bool enabled; // cursor style enabled
  using ModePropertyMap = std::map<std::string, msgpack::type::variant>;
  std::vector<ModePropertyMap> modeInfo;
  MSGPACK_DEFINE(enabled, modeInfo);
};
struct OptionSet {
  std::string name;
  msgpack::type::variant value;
  MSGPACK_DEFINE(name, value);
};
struct Chdir {
  std::string dir;
  MSGPACK_DEFINE(dir);
};
struct ModeChange {
  std::string mode;
  int modeIdx;
  MSGPACK_DEFINE(mode, modeIdx);
};
struct MouseOn {};
struct MouseOff {};
struct BusyStart {};
struct BusyStop {};
struct UpdateMenu {};
struct Flush {};
struct DefaultColorsSet {
  int rgbFg;
  int rgbBg;
  int rgbSp;
  int ctermFg;
  int ctermBg;
  MSGPACK_DEFINE(rgbFg, rgbBg, rgbSp, ctermFg, ctermBg);
};
struct HlAttrDefine {
  int id;
  using HlAttrMap = std::map<std::string, msgpack::type::variant>;
  HlAttrMap rgbAttrs;
  HlAttrMap ctermAttrs;
  std::vector<HlAttrMap> info;
  MSGPACK_DEFINE(id, rgbAttrs, ctermAttrs, info);
};
struct HlGroupSet {
  std::string name;
  int id;
  MSGPACK_DEFINE(name, id);
};
struct GridResize {
  int grid;
  int width;
  int height;
  MSGPACK_DEFINE(grid, width, height);
};
struct GridClear {
  int grid;
  MSGPACK_DEFINE(grid);
};
struct GridCursorGoto {
  int grid;
  int row;
  int col;
  MSGPACK_DEFINE(grid, row, col);
};
struct GridLine {
  struct Cell {
    std::string text;
    int hlId;
    int repeat = 1;
  };
  int grid;
  int row;
  int colStart;
  std::vector<Cell> cells;
};
struct GridScroll {
  int grid;
  int top;
  int bot;
  int left;
  int right;
  int rows;
  int cols;
  MSGPACK_DEFINE(grid, top, bot, left, right, rows, cols);
};
struct GridDestroy {
  int grid;
  MSGPACK_DEFINE(grid);
};
struct WinPos {
  int grid;
  msgpack::type::ext win;
  int startRow;
  int startCol;
  int width;
  int height;
  MSGPACK_DEFINE(grid, win, startRow, startCol, width, height);
};
struct WinFloatPos {
  int grid;
  msgpack::type::ext win;
  std::string anchor;
  int anchorGrid;
  float anchorRow;
  float anchorCol;
  bool focusable;
  int zindex;
  MSGPACK_DEFINE(
    grid, win, anchor, anchorGrid, anchorRow, anchorCol, focusable, zindex
  );
};
struct WinExternalPos {
  int grid;
  msgpack::type::ext win;
  MSGPACK_DEFINE(grid, win);
};
struct WinHide {
  int grid;
  MSGPACK_DEFINE(grid);
};
struct WinClose {
  int grid;
  MSGPACK_DEFINE(grid);
};
struct MsgSetPos {
  int grid;
  int row;
  bool scrolled;
  std::string sepChar;
  MSGPACK_DEFINE(grid, row, scrolled, sepChar);
};
struct WinViewport {
  int grid;
  msgpack::type::ext win;
  int topline;
  int botline;
  int curline;
  int curcol;
  int lineCount;
  int scrollDelta;
  MSGPACK_DEFINE(grid, win, topline, botline, curline, curcol, lineCount, scrollDelta);
};
struct WinViewportMargins {
  int grid;
  msgpack::type::ext win;
  int top;
  int bottom;
  int left;
  int right;
  MSGPACK_DEFINE(grid, win, top, bottom, left, right);
};
struct WinExtmark {
  int grid;
  msgpack::type::ext win;
  int nsId;
  int markId;
  int row;
  int col;
  MSGPACK_DEFINE(grid, win, nsId, markId, row, col);
};

using UiEvent = std::variant<
  SetTitle,
  SetIcon,
  ModeInfoSet,
  OptionSet,
  Chdir,
  ModeChange,
  MouseOn,
  MouseOff,
  BusyStart,
  BusyStop,
  UpdateMenu,
  DefaultColorsSet,
  HlAttrDefine,
  HlGroupSet,
  Flush,
  GridResize,
  GridClear,
  GridCursorGoto,
  GridLine,
  GridScroll,
  GridDestroy,
  WinPos,
  WinFloatPos,
  WinExternalPos,
  WinHide,
  WinClose,
  MsgSetPos,
  WinViewport,
  WinViewportMargins,
  WinExtmark>;

struct UiEvents {
  int numFlushes = 0;
  std::deque<std::vector<UiEvent>> queue;

  UiEvents() {
    queue.emplace_back();
  }

  auto& Curr() {
    return queue.back();
  }
};

void ParseUiEvent(const msgpack::object& params, UiEvents& uiEvents);
