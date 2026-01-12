
#include "lua_binding.h"
#include "embree_wrapper.h"
#include <iostream>
#include <SDL3/SDL.h>
#include <utility>

void bind_lua(sol::state& lua) {
    lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::math, sol::lib::string, sol::lib::table);

    // Provide a texture-based API. Preferred usage is to lock once, write many pixels, then unlock.
    // Legacy convenience: api_draw_pixel_texture still exists but callers should prefer lock/unlock + api_draw_pixel_locked.
    lua.set_function("api_draw_pixel_texture", [](void* texture, int x, int y, int r, int g, int b) {
        if (!texture) return;
        SDL_Texture* tex = static_cast<SDL_Texture*>(texture);
        void* pixels = nullptr;
        int pitch = 0;
        if (SDL_LockTexture(tex, NULL, &pixels, &pitch) == false) {
            return;
        }
        if (pixels) {
            uint32_t* pixel = (uint32_t*)((uint8_t*)pixels + y * pitch + x * sizeof(uint32_t));
            *pixel = (r) | (g << 8) | (b << 16) | (255 << 24);
        }
        SDL_UnlockTexture(tex);
    });

    // Lock/unlock API: lock returns (pixels, pitch) as two return values (pair).
    lua.set_function("api_lock_texture", [](void* texture) -> std::pair<void*, int> {
        if (!texture) return std::pair<void*, int>{nullptr, 0};
        SDL_Texture* tex = static_cast<SDL_Texture*>(texture);
        void* pixels = nullptr;
        int pitch = 0;
        if (SDL_LockTexture(tex, NULL, &pixels, &pitch) == false) {
            return std::pair<void*, int>{nullptr, 0};
        }
        return std::pair<void*, int>{pixels, pitch};
    });

    lua.set_function("api_unlock_texture", [](void* texture) {
        if (!texture) return;
        SDL_Texture* tex = static_cast<SDL_Texture*>(texture);
        SDL_UnlockTexture(tex);
    });

    // Draw into a locked pixel buffer: api_draw_pixel_locked(pixels, pitch, x, y, r, g, b)
    lua.set_function("api_draw_pixel_locked", [](void* pixels, int pitch, int x, int y, int r, int g, int b) {
        if (!pixels) return;
        uint32_t* pixel = (uint32_t*)((uint8_t*)pixels + y * pitch + x * sizeof(uint32_t));
        *pixel = (r) | (g << 8) | (b << 16) | (255 << 24);
    });

    // Create/destroy texture helpers. Lua creates texture from renderer lightuserdata.
    lua.set_function("api_create_texture", [](void* renderer, int w, int h) -> void* {
        if (!renderer) return nullptr;
        SDL_Renderer* rend = static_cast<SDL_Renderer*>(renderer);
        SDL_Texture* tex = SDL_CreateTexture(rend, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, w, h);
        return static_cast<void*>(tex);
    });

    lua.set_function("api_destroy_texture", [](void* texture) {
        if (!texture) return;
        SDL_Texture* tex = static_cast<SDL_Texture*>(texture);
        SDL_DestroyTexture(tex);
    });

    // Bind EmbreeDevice
    lua.new_usertype<EmbreeDevice>("EmbreeDevice",
        sol::constructors<EmbreeDevice()>(),
        "create_scene", [](EmbreeDevice& self) { return std::make_unique<EmbreeScene>(self); }
    );

    // Bind EmbreeScene
    lua.new_usertype<EmbreeScene>("EmbreeScene",
        "add_sphere", &EmbreeScene::add_sphere,
        "add_triangle", &EmbreeScene::add_triangle,
        "commit", &EmbreeScene::commit,
        "intersect", &EmbreeScene::intersect
    );
}

sol::object run_script(sol::state& lua, int argc, char* argv[]) {
    const char* scriptFile = "main.lua";
    if (argc > 1) {
        scriptFile = argv[1];
    }

    std::cout << "Running " << scriptFile << "..." << std::endl;
    sol::protected_function_result result = lua.script_file(scriptFile, sol::script_pass_on_error);
    if (!result.valid()) {
        sol::error err = result;
        std::cerr << "Lua Error: " << err.what() << std::endl;
        return sol::object(sol::nil);
    } else {
        std::cout << "Lua script executed successfully." << std::endl;
        return sol::object(result);
    }
}
