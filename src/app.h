#pragma once
#include <SDL3/SDL.h>

// Initialize SDL and create a renderer.
bool init_sdl(SDL_Window** window, SDL_Renderer** renderer);
#include <functional>
// Main loop renders using the provided texture.
void main_loop(SDL_Window* window, SDL_Renderer* renderer, SDL_Texture* texture, std::function<void()> on_frame = nullptr);
