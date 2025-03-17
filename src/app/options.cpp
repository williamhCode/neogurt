#include "./options.hpp"
#include "boost/program_options.hpp"
#include <iostream>

std::expected<AppOptions, int> AppOptions::LoadFromCommandLine(int argc, char** argv) {
  AppOptions options{};

  namespace po = boost::program_options;

  // command-line options
  po::options_description desc("Options");
  desc.add_options()
    ("help,h", "Show help message")
    ("version,V", "Show version")
    ("multigrid", po::value<bool>()->default_value(options.multigrid), "Use multigrid")
    ("interactiveShell,i", po::value<bool>()->default_value(options.interactiveShell), "Spawn neovim in an interactive shell")
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
    if (vm.count("multigrid")) {
      options.multigrid = vm["multigrid"].as<bool>();
    }
    if (vm.count("interactiveShell")) {
      options.interactiveShell = vm["interactiveShell"].as<bool>();
    }

  } catch (const po::error& ex) {
    std::cerr << "Error: " << ex.what() << "\n";
    return std::unexpected(1);
  }

  return options;
}
