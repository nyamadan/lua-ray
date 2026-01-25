#include <gtest/gtest.h>
#include "../src/lua_binding.h"
#include <sol/sol.hpp>

class LuaBindingTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Init Lua
        AppContext ctx;
        bind_lua(lua, ctx);
    }

    void TearDown() override {
        // sol::state cleans up itself
    }

    sol::state lua;
};

// Note: api_draw_pixel removed; texture-based API is available as api_draw_pixel_texture(texture, x,y,r,g,b)

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

TEST_F(LuaBindingTest, AddTriangle) {
    auto result = lua.safe_script(R"(
        local device = EmbreeDevice.new()
        local scene = device:create_scene()
        
        -- Add triangle should not crash
        scene:add_triangle(0, 0, 0, 1, 0, 0, 0, 1, 0)
        
        scene:commit()
        
        -- Intersect triangle
        -- Triangle is (0,0,0)-(1,0,0)-(0,1,0) (Z=0 plane)
        -- Ray from (0.2, 0.2, 1) direction (0, 0, -1) should hit
        local hit, t, nx, ny, nz = scene:intersect(0.2, 0.2, 1, 0, 0, -1)
        assert(hit == true)
        assert(math.abs(t - 1.0) < 0.001)
        -- Normal should be (0, 0, 1) or (0, 0, -1) depending on winding
        -- Counter-clockwise: (1,0,0)-(0,0,0) = (1,0,0), (0,1,0)-(0,0,0)=(0,1,0). Cross=(0,0,1).
        -- Wait, (0,0,0), (1,0,0), (0,1,0).
        -- v0=(0,0,0), v1=(1,0,0), v2=(0,1,0).
        -- e1 = v1-v0 = (1,0,0). e2 = v2-v0 = (0,1,0).
        -- cross(e1, e2) = (0, 0, 1).
        assert(math.abs(nz) > 0.9)
    )");
    ASSERT_TRUE(result.valid()) << ((sol::error)result).what();
}

TEST_F(LuaBindingTest, ExplicitRelease) {
    auto result = lua.safe_script(R"(
        local device = EmbreeDevice.new()
        local scene = device:create_scene()
        
        scene:add_sphere(0, 0, 0, 1.0)
        scene:commit()
        
        -- Release scene explicitly
        scene:release()
        
        -- Releasing again should be safe (idempotent)
        scene:release()
        
        -- Release device explicitly
        device:release()
        
        -- Releasing device again should be safe
        device:release()
        
        -- Create new scene after release should fail safely or be handled (here create_scene calls rtcNewScene(device), if device is released it might be null)
        -- In our wrapper, EmbreeScene checks if device is valid. If release() sets device=null, create_scene might create proper C++ obj but internal rtcScene will be null.
        -- Let's check internal state behavior if possible, or just ensure no crash.
        
        local device2 = EmbreeDevice.new()
        device2:release()
    )");
    ASSERT_TRUE(result.valid()) << ((sol::error)result).what();
}
