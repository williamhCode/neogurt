#include "path.hpp"

#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>

void InitResourcesDir() {
  CFBundleRef mainBundle = CFBundleGetMainBundle();
  CFURLRef bundleUrl = CFBundleCopyBundleURL(mainBundle);

  CFStringRef uti;
  if (
    CFURLCopyResourcePropertyForKey(
      bundleUrl, kCFURLTypeIdentifierKey, &uti, nullptr
    ) &&
    uti && UTTypeConformsTo(uti, kUTTypeApplicationBundle)
  ) {
    CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL(mainBundle);
    // Convert the URL to a file system path
    char path[PATH_MAX];
    if (CFURLGetFileSystemRepresentation(resourcesURL, TRUE, (UInt8*)path, PATH_MAX)) {
      CFRelease(resourcesURL);
      resourcesDir = path;
    }

  } else {
    resourcesDir = ROOT_DIR "/res";
  }
}
