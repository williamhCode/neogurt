#include "path.hpp"

#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>

void InitResourcesDir() {
  CFBundleRef mainBundle = CFBundleGetMainBundle();
  CFURLRef bundleUrl = CFBundleCopyBundleURL(mainBundle);
  
  CFStringRef uti = nullptr;
  Boolean isAppBundle = false;
  if (CFURLCopyResourcePropertyForKey(bundleUrl, kCFURLTypeIdentifierKey, &uti, nullptr) && uti) {
    isAppBundle = UTTypeConformsTo(uti, kUTTypeApplicationBundle);
    CFRelease(uti);
  }
  
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
  
  CFRelease(bundleUrl);
}
