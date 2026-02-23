#pragma once
#include <sol/sol.hpp>
#include <string>
#include <SDL3/SDL.h>

struct AppContext {
    int width = 800;
    int height = 600;
    int texture_width = 800;  // テクスチャ幅（スケーリング計算用）
    int texture_height = 600; // テクスチャ高さ（スケーリング計算用）
    std::string title = "Lua Ray Tracing";
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;
};

// Bind common types (AppData, Embree, GltfData) to any Lua state.
void bind_common_types(sol::state& lua);
// Bind Lua functions.
void bind_lua(sol::state& lua, AppContext& ctx);
// Bind Lua functions for worker threads.
void bind_worker_lua(sol::state& lua);
// Run script and return the script's return value (if any) as a sol::object.
sol::object run_script(sol::state& lua, int argc, char* argv[]);
