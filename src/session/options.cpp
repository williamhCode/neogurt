#include "./options.hpp"

#include "utils/logger.hpp"
#include "utils/async.hpp"
#include "boost/core/demangle.hpp"
#include "SDL3/SDL_hints.h"
#include <future>

static std::string CamelToSnake(std::string_view s) {
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

std::future<SessionOptions> SessionOptions::Load(Nvim& nvim) {
  SessionOptions options;

  auto opts_handle = co_await nvim.GetVar("neogurt_opts");
  std::map<std::string_view, msgpack::object> opts_obj;
  try {
    if (!opts_handle->convert_if_not_nil(opts_obj)) {
      co_return options;
    }
  } catch (const msgpack::type_error& e) {
    LOG_WARN("Failed to load neogurt_opts: expected lua table");
  }

  auto Load = [&] (std::string_view name, auto& value) {
    try {
      auto it = opts_obj.find(name);
      if (it == opts_obj.end()) {
        return;
      }
      it->second.convert_if_not_nil(value);

    } catch (const msgpack::type_error& e) {
      LOG_WARN(
        "Failed to load option '{}': expected type '{}'",
        name, boost::core::demangled_name(typeid(value))
      );

    } catch (const std::exception& e) {
      LOG_WARN("Failed to load option '{}': {}", name, e.what());
    }
  };
  #define LOAD(name) Load(CamelToSnake(#name), options.name)

  std::string macosOptionIsMetaString;

  LOAD(marginTop);
  LOAD(marginBottom);
  LOAD(marginLeft);
  LOAD(marginRight);
  LOAD(macosOptionIsMeta);
  LOAD(cursorIdleTime);
  LOAD(scrollSpeed);

  LOAD(bgColor);
  LOAD(opacity);

  LOAD(fps);

  SDL_SetHint(SDL_HINT_MAC_OPTION_AS_ALT, options.macosOptionIsMeta.c_str());

  options.opacity = int(options.opacity * 255) / 255.0f;

  co_return options;
}
