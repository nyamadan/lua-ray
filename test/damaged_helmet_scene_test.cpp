#include <gtest/gtest.h>
#include <sol/sol.hpp>
#include "app_data.h"
#include "embree_wrapper.h"
#include "lua_binding.h"

TEST(DamagedHelmetSceneTest, RendersCenterPixelThroughLifecycle) {
    sol::state lua;
    lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::math,
                       sol::lib::string, sol::lib::table);
    lua["package"]["path"] = "?.lua;?/init.lua;lib/?.lua;scenes/?.lua";
    bind_common_types(lua);

    AppData data(32, 32);
    EmbreeDevice device;
    EmbreeScene scene(device);
    lua["data"] = &data;
    lua["scene"] = &scene;
    auto result = lua.safe_script(R"(
        local helmet = require('scenes.damaged_helmet')
        helmet.setup(scene, data)
        scene:commit()
        helmet.start(scene, data)
        local camera = helmet.get_camera()
        assert(camera ~= nil)
        assert(math.abs(camera.position[1] - 1.15) < 0.001)
        assert(math.abs(camera.position[2] - 0.55) < 0.001)
        assert(math.abs(camera.position[3] - 3.0) < 0.001)
        helmet.shade(data, 16, 16)
        helmet.cleanup()
        return true
    )", sol::script_pass_on_error);
    ASSERT_TRUE(result.valid()) << sol::error(result).what();
    data.swap();
    auto [r, g, b] = data.get_pixel(16, 15);
    EXPECT_GT(r + g + b, 0);
}

TEST(DamagedHelmetSceneTest, PathTracedSceneAccumulatesOneSample) {
    sol::state lua;
    lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::math,
                       sol::lib::string, sol::lib::table);
    lua["package"]["path"] = "?.lua;?/init.lua;lib/?.lua;scenes/?.lua";
    bind_common_types(lua);
    AppData data(16, 16);
    EmbreeDevice device;
    EmbreeScene scene(device);
    lua["data"] = &data;
    lua["scene"] = &scene;
    auto result = lua.safe_script(R"(
        local helmet = require('scenes.damaged_helmet_pathtraced')
        helmet.setup(scene, data)
        scene:commit()
        data:set_string('progressive_pass', '1')
        helmet.start(scene, data)
        assert(helmet.is_progressive() and helmet.get_max_samples() == 128)
        helmet.shade(data, 8, 8)
        helmet.cleanup()
        return data:get_sample_count(8, 7)
    )", sol::script_pass_on_error);
    ASSERT_TRUE(result.valid()) << sol::error(result).what();
    EXPECT_EQ(result.get<int>(), 1);
}
