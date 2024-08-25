#include "options.hpp"
#include "utils/logger.hpp"
#include <format>
#include <future>
#include <boost/core/demangle.hpp>
#include "utils/async.hpp"

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

static std::future<void> LoadOption(Nvim& nvim, std::string_view name, auto& value) {
  try {
    auto luaCode = std::format("return vim.g.neogui_opts.{}", name);
    auto result = co_await nvim.ExecLua(luaCode, {});
    result->convert_if_not_nil(value);

  } catch (const msgpack::type_error& e) {
    LOG_ERR(
      "Failed to load option '{}': expected type '{}'",
      name, boost::core::demangled_name(typeid(value))
    );

  } catch (const std::exception& e) {
    LOG_ERR("Failed to load option '{}': {}", name, e.what());
  }
};

std::future<Options> LoadOptions(Nvim& nvim) {
  Options options;

  #define LOAD(name) LoadOption(nvim, CamelToSnakeCase(#name), options.name)

  // load options in parallel
  co_await WhenAll(
    LOAD(window.vsync),
    LOAD(window.highDpi),
    LOAD(window.borderless),
    LOAD(window.blur),

    LOAD(margins.top),
    LOAD(margins.bottom),
    LOAD(margins.left),
    LOAD(margins.right),

    LOAD(multigrid),
    LOAD(macOptIsMeta),
    LOAD(cursorIdleTime),
    LOAD(scrollSpeed),

    LOAD(bgColor),
    LOAD(opacity),

    LOAD(maxFps)
  );

  options.opacity = int(options.opacity * 255) / 255.0f;

  co_return options;
}
