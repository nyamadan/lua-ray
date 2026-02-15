
#include "lua_binding.h"
#include "imgui_lua_binding.h"
#include "embree_wrapper.h"
#include "gltf_loader.h"
#include <iostream>
#include "app.h"
#include "app_data.h"
#include "thread_worker.h"

// Helper to bind common types (AppData, Embree, GltfData) to any state
void bind_common_types(sol::state& lua) {
    // Bind EmbreeDevice
    lua.new_usertype<EmbreeDevice>("EmbreeDevice",
        sol::constructors<EmbreeDevice()>(),
        "create_scene", [](EmbreeDevice& self) { return std::make_unique<EmbreeScene>(self); },
        "release", &EmbreeDevice::release
    );

    // Bind EmbreeScene
    lua.new_usertype<EmbreeScene>("EmbreeScene",
        "add_sphere", &EmbreeScene::add_sphere,
        "add_triangle", &EmbreeScene::add_triangle,
        "add_mesh", &EmbreeScene::add_mesh,
        "commit", &EmbreeScene::commit,
        "intersect", &EmbreeScene::intersect,
        "release", &EmbreeScene::release
    );

    // Bind AppData
    lua.new_usertype<AppData>("AppData",
        sol::constructors<AppData(int, int)>(),
        "set_pixel", &AppData::set_pixel,
        "get_pixel", &AppData::get_pixel,
        "swap", &AppData::swap,
        "width", &AppData::get_width,
        "height", &AppData::get_height,
        "clear", &AppData::clear,
        "set_string", &AppData::set_string,
        "get_string", &AppData::get_string,
        "has_string", &AppData::has_string,
        "pop_next_index", &AppData::pop_next_index
    );

    // Bind GltfData (glTFファイル読み込み)
    lua.new_usertype<GltfData>("GltfData",
        sol::constructors<GltfData()>(),
        "load", &GltfData::load,
        "is_loaded", &GltfData::isLoaded,
        "get_mesh_count", &GltfData::getMeshCount,
        "get_vertices", &GltfData::getVertices,
        "get_indices", &GltfData::getIndices,
        "release", &GltfData::release
    );
}

void bind_worker_lua(sol::state& lua) {
    // Basic types
    bind_common_types(lua);
    
    // Create 'app' namespace for worker (limited functions)
    auto app = lua.create_named_table("app");
    
    // get_ticks is needed for timing/cancel check
    app.set_function("get_ticks", []() -> uint32_t {
        return SDL_GetTicks();
    });
}

void bind_lua(sol::state& lua, AppContext& ctx) {
    lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::math, sol::lib::string, sol::lib::table, sol::lib::coroutine, sol::lib::os, sol::lib::io);
    bind_imgui(lua);

    // Create 'app' namespace
    auto app = lua.create_named_table("app");

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
        // テクスチャサイズはwidth/heightと同じ値を使用（レンダリング解像度）
        ctx.texture_width = ctx.width;
        ctx.texture_height = ctx.height;
        ctx.title = cfg["title"].get_or(std::string("Lua Ray Tracing"));
        
        sol::object win_obj = cfg["window"];
        if (win_obj.is<void*>()) ctx.window = static_cast<SDL_Window*>(win_obj.as<void*>());
        
        sol::object ren_obj = cfg["renderer"];
        if (ren_obj.is<void*>()) ctx.renderer = static_cast<SDL_Renderer*>(ren_obj.as<void*>());
        
        sol::object tex_obj = cfg["texture"];
        if (tex_obj.is<void*>()) {
            ctx.texture = static_cast<SDL_Texture*>(tex_obj.as<void*>());
            // メインループのテクスチャも動的に更新
            set_active_texture(ctx.texture);
        }
    });

    // Provide a texture-based API
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

    // Update texture from AppData
    app.set_function("update_texture", [](void* texture, AppData& data) {
        if (!texture) return;
        SDL_Texture* tex = static_cast<SDL_Texture*>(texture);
        SDL_UpdateTexture(tex, NULL, data.get_data(), data.get_width() * sizeof(uint32_t));
    });

    // Update texture from AppData's back buffer (for in-progress rendering)
    app.set_function("update_texture_from_back", [](void* texture, AppData& data) {
        if (!texture) return;
        SDL_Texture* tex = static_cast<SDL_Texture*>(texture);
        SDL_UpdateTexture(tex, NULL, data.get_back_data(), data.get_width() * sizeof(uint32_t));
    });

    app.set_function("get_ticks", []() -> uint32_t {
        return SDL_GetTicks();
    });
    
    // Bind Common Types (Embree, AppData)
    bind_common_types(lua);
    
    // Bind ThreadWorker
    lua.new_usertype<ThreadWorker>("ThreadWorker",
        sol::constructors<ThreadWorker(AppData*, EmbreeScene*, ThreadWorker::Bounds, int)>(),
        "create", [](AppData* data, EmbreeScene* scene, int x, int y, int w, int h, int id) {
            return std::make_unique<ThreadWorker>(data, scene, ThreadWorker::Bounds{x, y, w, h}, id);
        },
        "start", &ThreadWorker::start,
        "join", &ThreadWorker::join,
        "terminate", &ThreadWorker::terminate,
        "is_done", &ThreadWorker::is_done,
        "is_cancel_requested", &ThreadWorker::is_cancel_requested,
        "get_progress", &ThreadWorker::get_progress
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
