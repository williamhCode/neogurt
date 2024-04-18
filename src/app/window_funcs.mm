#include "window_funcs.h"
#include "utils/logger.hpp"

#if defined(SDL_PLATFORM_MACOS)
#include <Cocoa/Cocoa.h>
#include <Foundation/Foundation.h>
#include <QuartzCore/CAMetalLayer.h>

extern "C" {
using CGSConnectionID = void*;
extern CGSConnectionID CGSMainConnectionID(void);
extern int CGSSetWindowBackgroundBlurRadius(CGSConnectionID connection, NSInteger windowNumber, NSInteger radius);
}
#endif

void SetSDLWindowBlur(SDL_Window* window, int blurRadius) {
#if defined(SDL_PLATFORM_MACOS)
  NSWindow* nswindow = (__bridge NSWindow*)SDL_GetProperty(
    SDL_GetWindowProperties(window), SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, NULL
  );
  if (nswindow) {
    NSInteger windowNumber = [nswindow windowNumber];
    CGSConnectionID conn = CGSMainConnectionID();
    CGSSetWindowBackgroundBlurRadius(conn, windowNumber, (NSInteger)blurRadius);
  }

#elif defined(SDL_PLATFORM_WIN32)
  HWND hwnd = (HWND)SDL_GetProperty(
    SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL
  );
  if (hwnd) {
    LOG_ERR("SetSDLWindowBlur: Not implemented for Windows");
  }

#elif defined(SDL_PLATFORM_LINUX)
  if (SDL_strcmp(SDL_GetCurrentVideoDriver(), "x11") == 0) {
    Display* xdisplay = (Display*)SDL_GetProperty(
      SDL_GetWindowProperties(window), SDL_PROP_WINDOW_X11_DISPLAY_POINTER, NULL
    );
    Window xwindow = (Window)SDL_GetNumberProperty(
      SDL_GetWindowProperties(window), SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0
    );
    if (xdisplay && xwindow) {
      LOG_ERR("SetSDLWindowBlur: Not implemented for X11");
    }
  } else if (SDL_strcmp(SDL_GetCurrentVideoDriver(), "wayland") == 0) {
    struct wl_display* display = (struct wl_display*)SDL_GetProperty(
      SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, NULL
    );
    struct wl_surface* surface = (struct wl_surface*)SDL_GetProperty(
      SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, NULL
    );
    if (display && surface) {
      LOG_ERR("SetSDLWindowBlur: Not implemented for Wayland");
    }
  }

#else
#error "Unsupported Target"
#endif
}
