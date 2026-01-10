#include <gtest/gtest.h>
#include "../src/lua_binding.h"
#include <sol/sol.hpp>

// Define global context for testing if not linked with app.cpp
// In unit tests we might want to mock this or ensure it's isolated.
// But since lua_binding depends on it, we need it.
#ifndef LINKED_WITH_APP
Context g_ctx;
#endif

class LuaBindingTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup dummy context
        g_ctx.width = 10;
        g_ctx.height = 10;
        g_ctx.pitch = 10 * 4;
        g_ctx.pixels = malloc(g_ctx.pitch * g_ctx.height);
        
        // Init Lua
        bind_lua(lua);
    }

    void TearDown() override {
        if (g_ctx.pixels) {
            free(g_ctx.pixels);
            g_ctx.pixels = nullptr;
        }
        // sol::state cleans up itself
    }

    sol::state lua;
};

TEST_F(LuaBindingTest, DrawPixelBoundsCheck) {
    // Valid call
    sol::protected_function_result result1 = lua.safe_script("api_draw_pixel(0, 0, 255, 0, 0)");
    ASSERT_TRUE(result1.valid());
    
    // Out of bounds - should not crash
    sol::protected_function_result result2 = lua.safe_script("api_draw_pixel(-1, 0, 255, 0, 0)");
    ASSERT_TRUE(result2.valid()); // It catches the error internally? No, returns valid status if no lua error.
    // Our C++ function just checks bounds and does nothing if invalid, so no lua error raised.
    
    sol::protected_function_result result3 = lua.safe_script("api_draw_pixel(100, 0, 255, 0, 0)");
    ASSERT_TRUE(result3.valid());
}

TEST_F(LuaBindingTest, EmbreeDeviceCreation) {
    auto result = lua.safe_script(R"(
        local device = EmbreeDevice.new()
        assert(device)
        local scene = device:create_scene()
        assert(scene)
    )");
    ASSERT_TRUE(result.valid()) << ((sol::error)result).what();
}

TEST_F(LuaBindingTest, EmbreeSceneOperations) {
    auto result = lua.safe_script(R"(
        local device = EmbreeDevice.new()
        local scene = device:create_scene()
        
        -- Add sphere should not crash
        scene:add_sphere(0, 0, 0, 1.0)
        
        scene:commit()
        
        -- Intersect
        local hit, t, nx, ny, nz = scene:intersect(0, 0, 5, 0, 0, -1)
        assert(hit == true)
        assert(t < 5)
    )");
    ASSERT_TRUE(result.valid()) << ((sol::error)result).what();
}
