#pragma once

struct SDL_Window;

void SetSDLWindowBlur(SDL_Window* sdlWindow, int blurRadius);
void EnableScrollMomentum();
void SetTransparentTitlebar(SDL_Window* sdlWindow);
float GetTitlebarHeight(SDL_Window* sdlWindow);
