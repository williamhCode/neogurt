#pragma once

#include "msgpack/object_fwd.hpp"
#include <unordered_map>
#include <string>

struct Grid {
  // data
  int width;
  int height;

  int cursorRow;
  int cursorCol;

  struct Cell {
    std::string text;
    int hl_id;
    int repeat = 1;
  };
  std::vector<Cell> newCells;

  // state
  bool resize;
  bool clear;
  bool cursorGoto;
  bool destroy;

  void UpdateState(Grid& source);
};

struct GridManager {
  std::unordered_map<int, Grid> grids;

  void UpdateState(GridManager& source);

  void Resize(const msgpack::object& args);
  void Clear(const msgpack::object& args);
  void CursorGoto(const msgpack::object& args);
  void Line(const msgpack::object& args);
  void Scroll(const msgpack::object& args);
  void Destroy(const msgpack::object& args);
};
