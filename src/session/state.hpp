#pragma once

#include "app/ime.hpp"
#include "app/input.hpp"
#include "nvim/nvim.hpp"
#include "editor/state.hpp"
#include "session/options.hpp"

struct Session {
  int id;
  std::string name;
  std::string dir;

  // session data ----------------------
  Nvim nvim;
  SessionOptions sessionOpts;
  EditorState editorState;
  InputHandler input;
  ImeHandler ime;
  bool reattached = false;
};
