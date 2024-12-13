#include "./options.hpp"

#include "boost/program_options.hpp"
#include <iostream>

#include "utils/logger.hpp"
#include <future>
#include <boost/core/demangle.hpp>
#include "utils/async.hpp"

std::optional<int> Options::LoadFromCommandLine(int argc, char** argv) {
  namespace po = boost::program_options;

  // Define command-line options
  po::options_description desc("Options");
  desc.add_options()
    ("help,h", "Show help message")
    ("version,V", "Show version")
    ("multigrid", po::value<bool>()->default_value(multigrid), "Use multigrid")
    ("interactiveShell,i", po::value<bool>()->default_value(interactiveShell), "Spawn neovim in an interactive shell")
  ;

  po::variables_map vm;

  try {
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
      std::cout << desc << "\n";
      return 0;
    }
    if (vm.count("version")) {
      std::cout << "Neogurt " VERSION "\n";
      return 0;
    }
    if (vm.count("multigrid")) {
      multigrid = vm["multigrid"].as<bool>();
    }
    if (vm.count("interactiveShell")) {
      interactiveShell = vm["interactiveShell"].as<bool>();
    }

  } catch (const po::error& ex) {
    std::cerr << "Error: " << ex.what() << "\n";
    std::cerr << desc << "\n";
    return 1;
  }

  return {};
}

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

std::future<Options> Options::Load(Nvim& nvim, bool first) {
  Options options;

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

  // TODO: load these options before connecting
  // LOAD(multigrid);
  // LOAD(interactiveShell);

  if (first) {
    LOAD(vsync);
    LOAD(highDpi);
    LOAD(borderless);
    LOAD(blur);
  }

  LOAD(marginTop);
  LOAD(marginBottom);
  LOAD(marginLeft);
  LOAD(marginRight);

  LOAD(macOptIsMeta);
  LOAD(cursorIdleTime);
  LOAD(scrollSpeed);

  LOAD(bgColor);
  LOAD(opacity);
  LOAD(gamma);

  LOAD(maxFps);

  LOAD(interactiveShell);

  options.opacity = int(options.opacity * 255) / 255.0f;

  co_return options;
}
