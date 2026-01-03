#include <SDL3/SDL.h>
#include <embree4/rtcore.h>
extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}
#include <iostream>
#include <vector>
#include <cmath>

// Global context for Lua access
struct Context {
    SDL_Texture* texture;
    void* pixels;
    int pitch;
    int width;
    int height;
};
Context g_ctx;

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
        *pixel = (r << 24) | (g << 16) | (b << 8) | 255; // RGBA? SDL_PIXELFORMAT_RGBA32 implies R G B A order usually, but let's stick to what worked or standarize. 
        // Actually SDL_PIXELFORMAT_RGBA32: Packed 32-bit: R, G, B, A order (ABGR in memory on little endian? Wait.)
        // Original code: (shade << 16) | (shade << 8) | shade | 0xFF000000; -> 0xAARRGGBB on little endian?
        // Let's use SDL_MapRGBA if we had surface, but here we write ints.
        // Assuming Little Endian (x86), 0xAABBGGRR.
        // SDL_PIXELFORMAT_RGBA32 is 4 bytes: R, G, B, A.
        // In integer on LE: 0xAABBGGRR.
        // Let's stick to the previous code's logic if possible, or correct it.
        // Previous code: *pixel = (shade << 16) | (shade << 8) | shade | 0xFF000000;
        // That looks like 0xFF is alpha (highest byte), then shade, shade, shade. 0xFFRR GGBB? No 0xFFrggbb.
        // Let's try explicit byte writing to be safe or just standard packing.
        // For RGBA32, bytes are R, G, B, A.
        // *pixel = R | (G << 8) | (B << 16) | (A << 24);
        *pixel = (r) | (g << 8) | (b << 16) | (255 << 24);
    }
    return 0;
}

// Embree Wrappers
int api_embree_new_device(lua_State* L) {
    RTCDevice device = rtcNewDevice(NULL);
    if (!device) {
        std::cerr << "Failed to create Embree device" << std::endl;
        return 0;
    }
    lua_pushlightuserdata(L, device);
    return 1;
}

int api_embree_new_scene(lua_State* L) {
    RTCDevice device = (RTCDevice)lua_touserdata(L, 1);
    RTCScene scene = rtcNewScene(device);
    lua_pushlightuserdata(L, scene);
    return 1;
}

int api_embree_add_sphere(lua_State* L) {
    RTCDevice device = (RTCDevice)lua_touserdata(L, 1);
    RTCScene scene = (RTCScene)lua_touserdata(L, 2);
    float cx = luaL_checknumber(L, 3);
    float cy = luaL_checknumber(L, 4);
    float cz = luaL_checknumber(L, 5);
    float r = luaL_checknumber(L, 6);

    RTCGeometry geom = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_SPHERE_POINT);
    float* buffer = (float*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT4, sizeof(float) * 4, 1);
    buffer[0] = cx;
    buffer[1] = cy;
    buffer[2] = cz;
    buffer[3] = r;

    rtcCommitGeometry(geom);
    rtcAttachGeometry(scene, geom);
    rtcReleaseGeometry(geom);
    return 0;
}

int api_embree_commit_scene(lua_State* L) {
    RTCScene scene = (RTCScene)lua_touserdata(L, 1);
    rtcCommitScene(scene);
    return 0;
}

int api_embree_intersect(lua_State* L) {
    RTCScene scene = (RTCScene)lua_touserdata(L, 1);
    float ox = luaL_checknumber(L, 2);
    float oy = luaL_checknumber(L, 3);
    float oz = luaL_checknumber(L, 4);
    float dx = luaL_checknumber(L, 5);
    float dy = luaL_checknumber(L, 6);
    float dz = luaL_checknumber(L, 7);

    RTCRayHit rayhit;
    rayhit.ray.org_x = ox; rayhit.ray.org_y = oy; rayhit.ray.org_z = oz;
    rayhit.ray.dir_x = dx; rayhit.ray.dir_y = dy; rayhit.ray.dir_z = dz;
    rayhit.ray.tnear = 0.0f;
    rayhit.ray.tfar = std::numeric_limits<float>::infinity();
    rayhit.ray.mask = -1;
    rayhit.ray.flags = 0;
    rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;

    rtcIntersect1(scene, &rayhit);

    if (rayhit.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
        lua_pushboolean(L, true);
        lua_pushnumber(L, rayhit.ray.tfar); // t
        
        // Normalize normal
        float nx = rayhit.hit.Ng_x;
        float ny = rayhit.hit.Ng_y;
        float nz = rayhit.hit.Ng_z;
        float len = sqrt(nx*nx + ny*ny + nz*nz);
        lua_pushnumber(L, nx/len);
        lua_pushnumber(L, ny/len);
        lua_pushnumber(L, nz/len);
        return 5;
    } else {
        lua_pushboolean(L, false);
        return 1;
    }
}

int api_embree_release_scene(lua_State* L) {
    RTCScene scene = (RTCScene)lua_touserdata(L, 1);
    rtcReleaseScene(scene);
    return 0;
}

int api_embree_release_device(lua_State* L) {
    RTCDevice device = (RTCDevice)lua_touserdata(L, 1);
    rtcReleaseDevice(device);
    return 0;
}


int main(int argc, char* argv[]) {
    // 1. Init SDL
    if (SDL_Init(SDL_INIT_VIDEO) == false) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        return 1;
    }

    g_ctx.width = 800;
    g_ctx.height = 600;
    SDL_Window* window = SDL_CreateWindow("Lua Ray Tracing", g_ctx.width, g_ctx.height, SDL_WINDOW_RESIZABLE);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    g_ctx.texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, g_ctx.width, g_ctx.height);

    if (!window || !renderer || !g_ctx.texture) {
        std::cerr << "SDL Setup failed" << std::endl;
        return 1;
    }

    // Lock texture for writing
    SDL_LockTexture(g_ctx.texture, NULL, &g_ctx.pixels, &g_ctx.pitch);

    // 2. Init Lua
    lua_State* L = luaL_newstate();
    luaL_openlibs(L);

    // Register API
    lua_register(L, "api_draw_pixel", api_draw_pixel);
    lua_register(L, "api_embree_new_device", api_embree_new_device);
    lua_register(L, "api_embree_new_scene", api_embree_new_scene);
    lua_register(L, "api_embree_add_sphere", api_embree_add_sphere);
    lua_register(L, "api_embree_commit_scene", api_embree_commit_scene);
    lua_register(L, "api_embree_intersect", api_embree_intersect);
    lua_register(L, "api_embree_release_scene", api_embree_release_scene);
    lua_register(L, "api_embree_release_device", api_embree_release_device);

    // 3. Run script
    std::cout << "Running raytracer.lua..." << std::endl;
    if (luaL_dofile(L, "raytracer.lua") != LUA_OK) {
        std::cerr << "Lua Error: " << lua_tostring(L, -1) << std::endl;
    } else {
        std::cout << "Lua script executed successfully." << std::endl;
    }

    lua_close(L);
    
    // Unlock texture
    SDL_UnlockTexture(g_ctx.texture);

    // 4. Output loop
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

    SDL_DestroyTexture(g_ctx.texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
