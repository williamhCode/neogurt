#pragma once

struct SDL_Window;

void SetSDLWindowBlur(SDL_Window* sdlWindow, int blurRadius);
void EnableScrollMomentum();
void SetTitlebarStyle(SDL_Window* sdlWindow, bool transparent);
float GetTitlebarHeight(SDL_Window* sdlWindow);
