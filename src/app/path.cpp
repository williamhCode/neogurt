#include "path.hpp"
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>

#include "utils/logger.hpp"
#include <chrono>

void SetupPaths() {
  CFBundleRef mainBundle = CFBundleGetMainBundle();
  CFURLRef bundleUrl = CFBundleCopyBundleURL(mainBundle);
  
  CFStringRef uti = nullptr;
  isAppBundle = false;
  if (CFURLCopyResourcePropertyForKey(bundleUrl, kCFURLTypeIdentifierKey, &uti, nullptr) && uti) {
    isAppBundle = UTTypeConformsTo(uti, kUTTypeApplicationBundle);
    CFRelease(uti);
  }
  
  // resources dir
  if (isAppBundle) {
    CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL(mainBundle);
    if (resourcesURL) {
      char path[PATH_MAX];
      if (CFURLGetFileSystemRepresentation(resourcesURL, TRUE, (UInt8*)path, PATH_MAX)) {
        resourcesDir = path;
      }
      CFRelease(resourcesURL);
    }
  } else {
    resourcesDir = ROOT_DIR "/res";
  }

  // logger stuff
  if (isAppBundle) {
    std::string homeDir = std::getenv("HOME");
    if (!homeDir.empty()) {
      fs::path logDir = fs::path(homeDir) / "Library" / "Logs" / "Neogurt";

      // Create directories if they don't exist
      if (!fs::exists(logDir)) {
        fs::create_directories(logDir);
      }

      using namespace std::chrono;
      // get date and time
      // TODO: fix later, clang not implemented yet, just use global time
      // auto time =
      //   current_zone()->to_local(system_clock::now());
      auto time = floor<seconds>(system_clock::now());

      logDir /= fs::path(std::format("{:%F at %I.%M.%S %p}.log", time));
      logger.RedirToPath(logDir);
    }
  }

  CFRelease(bundleUrl);
}
