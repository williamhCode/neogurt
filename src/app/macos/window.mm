#include "window.h"

#include <Cocoa/Cocoa.h>
#include <Foundation/Foundation.h>
#include <QuartzCore/CAMetalLayer.h>

extern "C" {
    using CGSConnectionID = void *;
    extern CGSConnectionID CGSMainConnectionID(void);
    extern int CGSSetWindowBackgroundBlurRadius(CGSConnectionID connection, NSInteger windowNumber, NSInteger radius);
}

NSWindow* GetNSWindowPointer(SDL_Window* window) {
  return (__bridge NSWindow *)SDL_GetProperty(
    SDL_GetWindowProperties(window), SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr
  );
}

void SetSDLWindowBlur(SDL_Window* window, int blurRadius) {
    NSWindow* ns_window = GetNSWindowPointer(window);

    NSInteger windowNumber = [ns_window windowNumber];
    CGSConnectionID conn = CGSMainConnectionID();
    CGSSetWindowBackgroundBlurRadius(conn, windowNumber, (NSInteger)blurRadius);
}

