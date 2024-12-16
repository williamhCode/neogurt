#pragma once

#include "nvim/nvim.hpp"
#include "editor/state.hpp"

struct SessionState {
  int id;
  std::string name;
  std::string dir;

  // session data ----------------------
  Nvim nvim;
  Options options;
  EditorState editorState;
  bool reattached = false;
};
