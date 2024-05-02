#pragma once
#include "app/input.hpp"
#include "editor/state.hpp"
#include "nvim/nvim.hpp"

struct NvimSession {
  Nvim nvim;
  EditorState editorState;
  InputHandler inputHandler;
};
