#pragma once
#include <SDL3/SDL.h>

struct Context {
    SDL_Texture* texture;
    void* pixels;
    int pitch;
    int width;
    int height;
};
