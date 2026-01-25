#pragma once
#include <sol/sol.hpp>
#include <string>
#include <SDL3/SDL.h>

struct AppContext {
    int width = 800;
    int height = 600;
    std::string title = "Lua Ray Tracing";
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;
};

// Bind Lua functions.
void bind_lua(sol::state& lua, AppContext& ctx);
// Run script and return the script's return value (if any) as a sol::object.
sol::object run_script(sol::state& lua, int argc, char* argv[]);
