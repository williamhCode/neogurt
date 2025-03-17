#include "./options.hpp"
#include "boost/program_options.hpp"
#include <iostream>

namespace po = boost::program_options;

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

std::expected<AppOptions, int> AppOptions::LoadFromCommandLine(int argc, char** argv) {
  AppOptions options{};

  #define DEFAULT_VAL(name) po::value<decltype(options.name)>()->default_value(options.name)

  // command-line options
  po::options_description desc("Options");
  desc.add_options()
    ("help,h", "Show help message")
    ("version,V", "Show version")

    ("interactive,i", DEFAULT_VAL(interactive), "Use interactive shell")
    ("multigrid", DEFAULT_VAL(multigrid), "Use multigrid")
    ("vsync", DEFAULT_VAL(vsync), "Use vsync")
    ("high_dpi", DEFAULT_VAL(highDpi), "Use high dpi")
    ("borderless", DEFAULT_VAL(borderless), "Use borderless window")
    ("blur", DEFAULT_VAL(blur), "Set window blur level")
  ;

  po::variables_map vm;

  // parse options
  try {
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
      std::cout << desc << "\n";
      return std::unexpected(0);
    }
    if (vm.count("version")) {
      std::cout << "Neogurt " VERSION "\n";
      return std::unexpected(0);
    }

    #define LOAD(name) {                                                               \
      auto snake = CamelToSnake(#name);                                                \
      if (vm.count(snake)) {                                                           \
        options.name = vm[snake].as<decltype(options.name)>();                         \
      }                                                                                \
    }

    LOAD(interactive);
    LOAD(multigrid);
    LOAD(vsync);
    LOAD(highDpi);
    LOAD(borderless);
    LOAD(blur);

  } catch (const po::error& ex) {
    std::cerr << "Error: " << ex.what() << "\n";
    return std::unexpected(1);
  }

  return options;
}
