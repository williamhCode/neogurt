#include "options.hpp"
#include "utils/logger.hpp"
#include <format>

static std::string CamelToSnakeCase(std::string_view s) {
  std::string result;
  for (char c : s) {
    if (isupper(c)) {
      result.push_back('_');
      result.push_back(tolower(c));
    } else {
      result.push_back(c);
    }
  }
  return result;
}

static void LoadOption(Nvim& nvim, std::string_view name, auto& value) {
  auto luaCode = std::format("return vim.g.neogui_opts.{}", CamelToSnakeCase(name));
  msgpack::object_handle result = nvim.ExecLua(luaCode, {});
  try {
    value = result->convert();
  } catch (const std::exception& e) {
    LOG_ERR("Failed to load option {}: {}", name, e.what());
  }
};

void Options::Load(Nvim& nvim) {
#define LOAD(name) LoadOption(nvim, #name, name)

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
