#include "app.h"
#include <iostream>

bool init_sdl(SDL_Window** window, SDL_Renderer** renderer) {
    if (SDL_Init(SDL_INIT_VIDEO) == false) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        return false;
    }

    int width = 800;
    int height = 600;
    *window = SDL_CreateWindow("Lua Ray Tracing", width, height, SDL_WINDOW_RESIZABLE);
    *renderer = SDL_CreateRenderer(*window, NULL);

    if (!*window || !*renderer) {
        std::cerr << "SDL Setup failed" << std::endl;
        return false;
    }
    return true;
}

void main_loop(SDL_Renderer* renderer, SDL_Texture* texture) {
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
        if (texture) {
            SDL_RenderTexture(renderer, texture, NULL, NULL);
        }
        SDL_RenderPresent(renderer);
    }
}
