#pragma once
#include <string_view>

struct SDL_Window;

void SetSDLWindowBlur(SDL_Window* sdlWindow, int blurRadius);
void ShowTitle(SDL_Window* sdlWindow, bool show);
void SetTitlebarStyle(SDL_Window* sdlWindow, std::string_view titlebar);
float GetTitlebarHeight(SDL_Window* sdlWindow);
