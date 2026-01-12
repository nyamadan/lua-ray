#pragma once
#include <SDL3/SDL.h>

// Initialize SDL and create a renderer.
bool init_sdl(SDL_Window** window, SDL_Renderer** renderer);
// Main loop renders using the provided texture.
void main_loop(SDL_Renderer* renderer, SDL_Texture* texture);
