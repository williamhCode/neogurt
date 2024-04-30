#pragma once
#include "app/input.hpp"
#include "editor/state.hpp"
#include "nvim/client.hpp"

struct NvimSession {
  Nvim nvim;
  EditorState editorState;
  InputHandler inputHandler;
};
