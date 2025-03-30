#include "./window_funcs.h"
#include "utils/logger.hpp"
#include "SDL3/SDL.h"

#if defined(SDL_PLATFORM_MACOS)
#include <Cocoa/Cocoa.h>
#include <Foundation/Foundation.h>
#include <QuartzCore/CAMetalLayer.h>

extern "C" {
  using CGSConnectionID = void*;
  extern CGSConnectionID CGSMainConnectionID(void);
  extern int CGSSetWindowBackgroundBlurRadius(
    CGSConnectionID connection, NSInteger windowNumber, NSInteger radius
  );
}

static NSWindow* GetNSWindow(SDL_Window* window) {
  return (__bridge NSWindow*)SDL_GetPointerProperty(
    SDL_GetWindowProperties(window), SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, NULL
  );
}
#endif

void SetSDLWindowBlur(SDL_Window* sdlWindow, int blurRadius) {
#if defined(SDL_PLATFORM_MACOS)
  auto *window = GetNSWindow(sdlWindow);
  NSInteger windowNumber = [window windowNumber];
  CGSConnectionID conn = CGSMainConnectionID();
  CGSSetWindowBackgroundBlurRadius(conn, windowNumber, (NSInteger)blurRadius);

#elif defined(SDL_PLATFORM_WIN32)
  HWND hwnd = (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
  if (hwnd) {
    LOG_ERR("SetSDLWindowBlur: Not implemented for Windows");
  }

#elif defined(SDL_PLATFORM_LINUX)
  if (SDL_strcmp(SDL_GetCurrentVideoDriver(), "x11") == 0) {
    Display *xdisplay = (Display *)SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_X11_DISPLAY_POINTER, NULL);
    Window xwindow = (Window)SDL_GetNumberProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
    if (xdisplay && xwindow) {
      LOG_ERR("SetSDLWindowBlur: Not implemented for x11");
    }
  } else if (SDL_strcmp(SDL_GetCurrentVideoDriver(), "wayland") == 0) {
    struct wl_display *display = (struct wl_display *)SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, NULL);
    struct wl_surface *surface = (struct wl_surface *)SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, NULL);
    if (display && surface) {
      LOG_ERR("SetSDLWindowBlur: Not implemented for wayland");
    }
  }

#else
#error "Unsupported Target"
#endif
}

// call in main thread only
void ShowTitle(SDL_Window* sdlWindow, bool show) {
  NSWindow* window = GetNSWindow(sdlWindow);
  if (show) {
    window.titleVisibility = NSWindowTitleVisible;
  } else {
    window.titleVisibility = NSWindowTitleHidden;
  }
}

void SetTitlebarStyle(SDL_Window* sdlWindow, std::string_view titlebar) {
  NSWindow* window = GetNSWindow(sdlWindow);

  if (titlebar == "transparent") {
    window.styleMask |= NSWindowStyleMaskFullSizeContentView;
    window.titlebarAppearsTransparent = true;
    [[window standardWindowButton:NSWindowCloseButton] setHidden:NO];
    [[window standardWindowButton:NSWindowMiniaturizeButton] setHidden:NO];
    [[window standardWindowButton:NSWindowZoomButton] setHidden:NO];

  } else if (titlebar == "none") {
    window.styleMask |= NSWindowStyleMaskFullSizeContentView;
    window.titlebarAppearsTransparent = true;
    [[window standardWindowButton:NSWindowCloseButton] setHidden:YES];
    [[window standardWindowButton:NSWindowMiniaturizeButton] setHidden:YES];
    [[window standardWindowButton:NSWindowZoomButton] setHidden:YES];

  } else {
    window.styleMask &= ~NSWindowStyleMaskFullSizeContentView;
    window.titlebarAppearsTransparent = false;
    [[window standardWindowButton:NSWindowCloseButton] setHidden:NO];
    [[window standardWindowButton:NSWindowMiniaturizeButton] setHidden:NO];
    [[window standardWindowButton:NSWindowZoomButton] setHidden:NO];
  }
}

// call in main thread only
float GetTitlebarHeight(SDL_Window* sdlWindow) {
  NSWindow* window = GetNSWindow(sdlWindow);
  NSRect fullFrame = [window frame];
  NSRect contentRect = [window contentLayoutRect];
  CGFloat titleBarHeight = NSHeight(fullFrame) - NSHeight(contentRect);
  return titleBarHeight;
}
