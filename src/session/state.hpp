#pragma once

#include "nvim/nvim.hpp"
#include "editor/state.hpp"
#include "app/input.hpp"

#include "app/options.hpp"
#include "app/size.hpp"

struct SessionState {
  int id;
  std::string name;

  // session data ----------------------
  Nvim nvim;
  EditorState editorState;
  InputHandler inputHandler;
};
