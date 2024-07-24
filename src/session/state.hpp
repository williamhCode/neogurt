#pragma once

#include "app/input.hpp"
#include "app/options.hpp"
#include "app/size.hpp"
#include "editor/state.hpp"
#include "nvim/nvim.hpp"
#include "utils/color.hpp"
#include <stdexcept>

struct SessionState {
  int id;
  std::string name;

  // session data ----------------------
  Nvim nvim;
  EditorState editorState;
  InputHandler inputHandler;

  // -------------------------------------
  void InitNvim(Options& options) {
    if (!nvim.ConnectStdio()) {
      throw std::runtime_error("Failed to connect to nvim");
    }

    nvim.UiAttach(
      100, 50,
      {
        {"rgb", true},
        {"ext_multigrid", true},
        {"ext_linegrid", true},
      }
    ).wait();

    options.Load(nvim);
  }

  void LoadFont(float dpi) {
    std::string guifont = nvim.GetOptionValue("guifont", {}).get()->convert();
    auto fontFamilyResult = FontFamily::FromGuifont(guifont, dpi);
    if (!fontFamilyResult) {
      throw std::runtime_error("Failed to create font family: " + fontFamilyResult.error());
    }
    editorState.fontFamily = std::move(*fontFamilyResult);
  }

  void InitEditorState(const Options& options, const SizeHandler& sizes) {
    editorState.Init(sizes);

    if (options.transparency < 1) {
      auto& hl = editorState.hlTable[0];
      hl.background = IntToColor(options.bgColor);
      hl.background->a = options.transparency;
    }
  }

  void InitInputHandler(const Options& options) {
    inputHandler = InputHandler(
      &nvim, &editorState.winManager, options.macOptIsMeta, options.multigrid,
      options.scrollSpeed
    );
  }
};
