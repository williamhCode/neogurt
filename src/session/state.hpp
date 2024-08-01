#pragma once

#include "nvim/nvim.hpp"
#include "editor/state.hpp"

struct SessionState {
  int id;
  std::string name;

  // session data ----------------------
  Nvim nvim;
  EditorState editorState;
  bool reattached = false;
};
