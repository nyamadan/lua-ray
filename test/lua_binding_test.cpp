#include <gtest/gtest.h>
#include "../src/lua_binding.h"

// Define global context for testing if not linked with app.cpp
// In unit tests we might want to mock this or ensure it's isolated.
// But since lua_binding depends on it, we need it.
#ifndef LINKED_WITH_APP
Context g_ctx;
#endif

class LuaBindingTest : public ::testing::Test {
protected:
    void SetUp() override {
        L = luaL_newstate();
        luaL_openlibs(L);
        register_lua_api(L);
        
        // Setup dummy context
        g_ctx.width = 10;
        g_ctx.height = 10;
        g_ctx.pitch = 10 * 4;
        g_ctx.pixels = malloc(g_ctx.pitch * g_ctx.height);
    }

    void TearDown() override {
        lua_close(L);
        free(g_ctx.pixels);
    }

    lua_State* L;
};

TEST_F(LuaBindingTest, DrawPixelBoundsCheck) {
    // Valid call
    luaL_dostring(L, "api_draw_pixel(0, 0, 255, 0, 0)");
    
    // Out of bounds - should not crash
    luaL_dostring(L, "api_draw_pixel(-1, 0, 255, 0, 0)");
    luaL_dostring(L, "api_draw_pixel(100, 0, 255, 0, 0)");
}

TEST_F(LuaBindingTest, EmbreeDeviceCreation) {
    luaL_dostring(L, "device = api_embree_new_device()");
    lua_getglobal(L, "device");
    ASSERT_TRUE(lua_islightuserdata(L, -1));
    
    luaL_dostring(L, "api_embree_release_device(device)");
}
