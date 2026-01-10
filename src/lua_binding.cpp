#include "lua_binding.h"
#include "embree_wrapper.h"
#include <iostream>

// Lua Binding: Draw Pixel
// draw_pixel(x, y, r, g, b)
void draw_pixel(int x, int y, int r, int g, int b) {
    if (x >= 0 && x < g_ctx.width && y >= 0 && y < g_ctx.height) {
        uint32_t* pixel = (uint32_t*)((uint8_t*)g_ctx.pixels + y * g_ctx.pitch + x * sizeof(uint32_t));
        // SDL_PIXELFORMAT_RGBA32 packs as R, G, B, A order.
        // On Little Endian (x86), this acts as 0xAABBGGRR in integer representation.
        *pixel = (r) | (g << 8) | (b << 16) | (255 << 24);
    }
}

void bind_lua(sol::state& lua) {
    lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::math, sol::lib::string, sol::lib::table);

    // Bind draw_pixel
    lua.set_function("api_draw_pixel", draw_pixel);

    // Bind EmbreeDevice
    lua.new_usertype<EmbreeDevice>("EmbreeDevice",
        sol::constructors<EmbreeDevice()>(),
        "create_scene", [](EmbreeDevice& self) { return std::make_unique<EmbreeScene>(self); }
    );

    // Bind EmbreeScene
    // Note: We return unique_ptr for create_scene, so Lua takes ownership.
    lua.new_usertype<EmbreeScene>("EmbreeScene",
        // No constructor exposed directly if we want to enforce creation via device, 
        // but sol::constructors is usually good. 
        // For now, let's allow creation via device:create_scene only, so no constructors here?
        // Actually, if we want to allow `EmbreeScene.new` we need one.
        // But the plan was `device:create_scene()`.
        
        "add_sphere", &EmbreeScene::add_sphere,
        "commit", &EmbreeScene::commit,
        "intersect", &EmbreeScene::intersect
    );
}

void run_script(sol::state& lua, int argc, char* argv[]) {
    const char* scriptFile = "main.lua";
    if (argc > 1) {
        scriptFile = argv[1];
    }

    std::cout << "Running " << scriptFile << "..." << std::endl;
    sol::protected_function_result result = lua.script_file(scriptFile, sol::script_pass_on_error);
    if (!result.valid()) {
        sol::error err = result;
        std::cerr << "Lua Error: " << err.what() << std::endl;
    } else {
        std::cout << "Lua script executed successfully." << std::endl;
    }
}
