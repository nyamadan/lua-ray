
#include "lua_binding.h"
#include "imgui_lua_binding.h"
#include "embree_wrapper.h"
#include <iostream>
#include "app.h"

void bind_lua(sol::state& lua, AppContext& ctx) {
    lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::math, sol::lib::string, sol::lib::table);
    bind_imgui(lua);

    // Create 'app' namespace
    auto app = lua.create_named_table("app");

    // 1. Init Video
    // 1. Init Video
    app.set_function("init_video", [&]() -> bool {
        if (SDL_Init(SDL_INIT_VIDEO) == false) {
            std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
            return false;
        }
        return true;
    });

    // 2. Create Window (Factory)
    app.set_function("create_window", [&](int width, int height, std::string title) -> void* {
        SDL_Window* window = SDL_CreateWindow(title.c_str(), width, height, SDL_WINDOW_RESIZABLE);
        if (!window) {
            std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
            return nullptr;
        }
        return static_cast<void*>(window);
    });

    // 3. Create Renderer (Factory)
    app.set_function("create_renderer", [&](void* window) -> void* {
        if (!window) return nullptr;
        SDL_Window* win = static_cast<SDL_Window*>(window);
        SDL_Renderer* renderer = SDL_CreateRenderer(win, NULL);
        if (!renderer) {
            std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << std::endl;
            return nullptr;
        }
        return static_cast<void*>(renderer);
    });

    // 4. Create Texture (Factory)
    app.set_function("create_texture", [&](void* renderer, int width, int height) -> void* {
        if (!renderer) return nullptr;
        SDL_Renderer* rend = static_cast<SDL_Renderer*>(renderer);
        SDL_Texture* tex = SDL_CreateTexture(rend, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, width, height);
        if (!tex) {
            std::cerr << "SDL_CreateTexture failed: " << SDL_GetError() << std::endl;
            return nullptr;
        }
        return static_cast<void*>(tex);
    });

    // 5. Configure (Dependency Injection)
    app.set_function("configure", [&](sol::table cfg) {
        ctx.width = cfg["width"].get_or(800);
        ctx.height = cfg["height"].get_or(600);
        ctx.title = cfg["title"].get_or(std::string("Lua Ray Tracing"));
        
        sol::object win_obj = cfg["window"];
        if (win_obj.is<void*>()) ctx.window = static_cast<SDL_Window*>(win_obj.as<void*>());
        
        sol::object ren_obj = cfg["renderer"];
        if (ren_obj.is<void*>()) ctx.renderer = static_cast<SDL_Renderer*>(ren_obj.as<void*>());
        
        sol::object tex_obj = cfg["texture"];
        if (tex_obj.is<void*>()) ctx.texture = static_cast<SDL_Texture*>(tex_obj.as<void*>());
    });

    // Provide a texture-based API. Preferred usage is to lock once, write many pixels, then unlock.
    app.set_function("draw_pixel_texture", [](void* texture, int x, int y, int r, int g, int b) {
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
    app.set_function("lock_texture", [](void* texture) -> std::pair<void*, int> {
        if (!texture) return std::pair<void*, int>{nullptr, 0};
        SDL_Texture* tex = static_cast<SDL_Texture*>(texture);
        void* pixels = nullptr;
        int pitch = 0;
        if (SDL_LockTexture(tex, NULL, &pixels, &pitch) == false) {
            return std::pair<void*, int>{nullptr, 0};
        }
        return std::pair<void*, int>{pixels, pitch};
    });

    app.set_function("unlock_texture", [](void* texture) {
        if (!texture) return;
        SDL_Texture* tex = static_cast<SDL_Texture*>(texture);
        SDL_UnlockTexture(tex);
    });

    // Draw into a locked pixel buffer: draw_pixel_locked(pixels, pitch, x, y, r, g, b)
    app.set_function("draw_pixel_locked", [](void* pixels, int pitch, int x, int y, int r, int g, int b) {
        if (!pixels) return;
        uint32_t* pixel = (uint32_t*)((uint8_t*)pixels + y * pitch + x * sizeof(uint32_t));
        *pixel = (r) | (g << 8) | (b << 16) | (255 << 24);
    });



    app.set_function("destroy_texture", [](void* texture) {
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
