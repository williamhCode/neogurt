#include "./locator.hpp"
#include "SDL3/SDL_platform_defines.h"

#if defined(SDL_PLATFORM_MACOS)
#include "locator_macos.mm"

#elif defined(SDL_PLATFORM_WIN32)
#include "locator_win32.cpp"

#elif defined(SDL_PLATFORM_LINUX)
#include "locator_linux.cpp"

#else
#error "Unsupported Platform"
#endif
