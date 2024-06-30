#include "options.hpp"
#include "utils/logger.hpp"
#include <format>

static void LoadOption(Nvim& nvim, std::string_view name, auto& value) {
  auto luaCode = std::format("return vim.g.neogui_opts_resolved.{}", name);
  msgpack::object_handle result = nvim.ExecLua(luaCode, {});
  try {
    value = result->convert();
  } catch (const std::exception& e) {
    LOG_ERR("Failed to load option {}: {}", name, e.what());
  }
};

#define LOAD(name) LoadOption(nvim, #name, name)

void Options::Load(Nvim& nvim) {
  nvim.ExecLua("vim.g.resolve_neogui_opts()", {});

  LOAD(window.vsync);
  LOAD(window.highDpi);
  LOAD(window.borderless);
  LOAD(window.blur);

  LOAD(margins.top);
  LOAD(margins.bottom);
  LOAD(margins.left);
  LOAD(margins.right);

  LOAD(multigrid);
  LOAD(macOptIsMeta);
  LOAD(cursorIdleTime);

  LOAD(bgColor);
  LOAD(transparency);

  LOAD(maxFps);

  transparency = int(transparency * 255) / 255.0f;
}
