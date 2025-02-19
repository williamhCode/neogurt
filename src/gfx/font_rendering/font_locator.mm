#include "./font_locator.hpp"
#include "SDL3/SDL_platform_defines.h"

#if defined(SDL_PLATFORM_MACOS)
#include "./font_locator_macos.mm"

#elif defined(SDL_PLATFORM_WIN32)
#include "./font_locator_win32.cpp"

#elif defined(SDL_PLATFORM_LINUX)
#include "./font_locator_linux.cpp"

#else
#error "Unsupported Platform"
#endif
