#include <gtest/gtest.h>
#include "../src/lua_binding.h"
#include <sol/sol.hpp>

class RayTracerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Init Lua
        AppContext ctx;
        bind_lua(lua, ctx);
        
        // Add lib directory to package.path (relative to build/gcc-debug)
        lua.script("package.path = package.path .. ';./lib/?.lua;../../?.lua'");
    }

    void TearDown() override {
    }

    sol::state lua;
};

TEST_F(RayTracerTest, RayTracerClassExists) {
    auto result = lua.safe_script("require('lib.RayTracer'); return RayTracer");
    ASSERT_TRUE(result.valid());
    ASSERT_TRUE(result.get<sol::object>().is<sol::table>());
}

TEST_F(RayTracerTest, NewInstance) {
    auto result = lua.safe_script(R"(
        local RayTracer = require('lib.RayTracer')
        local rt = RayTracer.new(800, 600)
        return rt.width, rt.height
    )");
    ASSERT_TRUE(result.valid());
    std::tuple<int, int> size = result;
    ASSERT_EQ(std::get<0>(size), 800);
    ASSERT_EQ(std::get<1>(size), 600);
}

TEST_F(RayTracerTest, InitSuccess) {
    // Mock app functions
    lua.script(R"(
        app.init_video = function() return true end
        app.create_window = function(w, h, title) return "mock_window" end
        app.create_renderer = function(win) return "mock_renderer" end
        app.create_texture = function(r, w, h) return "mock_texture" end
        app.configure = function(config) 
            _G.configured = true
            _G.config_title = config.title
        end
        -- EmbreeDevice is real, but that's fine as confirmed by other tests
    )");

    auto result = lua.safe_script(R"(
        local RayTracer = require('lib.RayTracer')
        local rt = RayTracer.new(800, 600)
        rt:init()
        return rt.window, rt.renderer, rt.texture, _G.configured
    )");
    
    ASSERT_TRUE(result.valid()) << ((sol::error)result).what();
    std::tuple<std::string, std::string, std::string, bool> res = result;
    ASSERT_EQ(std::get<0>(res), "mock_window");
    ASSERT_EQ(std::get<1>(res), "mock_renderer");
    ASSERT_EQ(std::get<2>(res), "mock_texture");
    ASSERT_TRUE(std::get<3>(res));
}
