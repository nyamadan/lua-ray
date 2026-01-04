#pragma once
#include <SDL3/SDL.h>
#include "context.h"

// Global context definition
extern Context g_ctx;

bool init_sdl(SDL_Window** window, SDL_Renderer** renderer);
void main_loop(SDL_Renderer* renderer);
