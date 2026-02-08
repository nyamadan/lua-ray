#pragma once
#include <SDL3/SDL.h>

// Initialize SDL and create a renderer.
bool init_sdl(SDL_Window** window, SDL_Renderer** renderer, int width, int height, const char* title);
#include <functional>
// Main loop renders using the provided texture.
void main_loop(SDL_Window* window, SDL_Renderer* renderer, SDL_Texture* texture, std::function<void()> on_frame = nullptr, std::function<void()> on_quit = nullptr);
// Update the texture used by the main loop dynamically.
void set_active_texture(SDL_Texture* texture);
