#include "lua_binding.h"
#include "embree_wrapper.h"
#include <iostream>

// Lua Binding: Draw Pixel
// api_draw_pixel(x, y, r, g, b)
int api_draw_pixel(lua_State* L) {
    int x = luaL_checkinteger(L, 1);
    int y = luaL_checkinteger(L, 2);
    int r = luaL_checkinteger(L, 3);
    int g = luaL_checkinteger(L, 4);
    int b = luaL_checkinteger(L, 5);

    if (x >= 0 && x < g_ctx.width && y >= 0 && y < g_ctx.height) {
        uint32_t* pixel = (uint32_t*)((uint8_t*)g_ctx.pixels + y * g_ctx.pitch + x * sizeof(uint32_t));
        // SDL_PIXELFORMAT_RGBA32 packs as R, G, B, A order.
        // On Little Endian (x86), this acts as 0xAABBGGRR in integer representation.
        *pixel = (r) | (g << 8) | (b << 16) | (255 << 24);
    }
    return 0;
}

void register_lua_api(lua_State* L) {
    lua_register(L, "api_draw_pixel", api_draw_pixel);
    lua_register(L, "api_embree_new_device", api_embree_new_device);
    lua_register(L, "api_embree_new_scene", api_embree_new_scene);
    lua_register(L, "api_embree_add_sphere", api_embree_add_sphere);
    lua_register(L, "api_embree_commit_scene", api_embree_commit_scene);
    lua_register(L, "api_embree_intersect", api_embree_intersect);
    lua_register(L, "api_embree_release_scene", api_embree_release_scene);
    lua_register(L, "api_embree_release_device", api_embree_release_device);
}

lua_State* init_lua() {
    lua_State* L = luaL_newstate();
    luaL_openlibs(L);
    register_lua_api(L);
    return L;
}

void run_script(lua_State* L, int argc, char* argv[]) {
    const char* scriptFile = "main.lua";
    if (argc > 1) {
        scriptFile = argv[1];
    }

    std::cout << "Running " << scriptFile << "..." << std::endl;
    if (luaL_dofile(L, scriptFile) != LUA_OK) {
        std::cerr << "Lua Error: " << lua_tostring(L, -1) << std::endl;
    } else {
        std::cout << "Lua script executed successfully." << std::endl;
    }
}
