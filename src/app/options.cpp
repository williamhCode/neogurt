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

#define LOAD_OPTION(name) LoadOption(nvim, #name, name)

void Options::Load(Nvim& nvim) {
  nvim.ExecLua("vim.g.resolve_neogui_opts()", {});

  LOAD_OPTION(window.vsync);
  LOAD_OPTION(window.highDpi);
  LOAD_OPTION(window.borderless);
  LOAD_OPTION(window.blur);

  LOAD_OPTION(margins.top);
  LOAD_OPTION(margins.bottom);
  LOAD_OPTION(margins.left);
  LOAD_OPTION(margins.right);

  LOAD_OPTION(multigrid);
  LOAD_OPTION(macOptAsAlt);
  LOAD_OPTION(cursorIdleTime);

  LOAD_OPTION(bgColor);
  LOAD_OPTION(transparency);
  transparency = int(transparency * 255) / 255.0f;
}
