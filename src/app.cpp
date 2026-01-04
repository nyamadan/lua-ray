#include "app.h"
#include <iostream>

Context g_ctx;

bool init_sdl(SDL_Window** window, SDL_Renderer** renderer) {
    if (SDL_Init(SDL_INIT_VIDEO) == false) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        return false;
    }

    g_ctx.width = 800;
    g_ctx.height = 600;
    *window = SDL_CreateWindow("Lua Ray Tracing", g_ctx.width, g_ctx.height, SDL_WINDOW_RESIZABLE);
    *renderer = SDL_CreateRenderer(*window, NULL);
    g_ctx.texture = SDL_CreateTexture(*renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, g_ctx.width, g_ctx.height);

    if (!*window || !*renderer || !g_ctx.texture) {
        std::cerr << "SDL Setup failed" << std::endl;
        return false;
    }
    return true;
}

void main_loop(SDL_Renderer* renderer) {
    bool running = true;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_RenderTexture(renderer, g_ctx.texture, NULL, NULL);
        SDL_RenderPresent(renderer);
    }
}
