#include <gtest/gtest.h>
#include <sol/sol.hpp>
#include <tuple>

class CameraUtilsTest : public ::testing::Test {
protected:
    void SetUp() override {
        lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::math, sol::lib::table, sol::lib::string);
        // Add lib directory to package.path
        lua.script("package.path = package.path .. ';./lib/?.lua;../../?.lua'");
    }

    sol::state lua;
};

// 1. 新規カメラの生成テスト
TEST_F(CameraUtilsTest, SetupNewCamera) {
    auto script = R"(
        local CameraUtils = require('lib.CameraUtils')
        local default_params = {
            position = {0, 1.0, 5.0},
            look_at = {0, 0, 0},
            up = {0, 1, 0},
            aspect_ratio = 1.0,
            fov = 60.0
        }
        local app_data = {
            get_string = function(self, key) return nil end
        }
        
        local camera = CameraUtils.setup_or_sync_camera(nil, app_data, default_params)
        
        return camera ~= nil, camera.position[3], camera.fov
    )";
    auto result = lua.safe_script(script);
    ASSERT_TRUE(result.valid()) << ((sol::error)result).what();
    auto [is_created, z, fov] = result.get<std::tuple<bool, double, double>>();
    ASSERT_TRUE(is_created);
    ASSERT_EQ(z, 5.0);
    ASSERT_EQ(fov, 60.0);
}

// 2. JSONから状態復元テスト
TEST_F(CameraUtilsTest, RestoreCameraFromJson) {
    auto script = R"(
        local CameraUtils = require('lib.CameraUtils')
        
        local Camera = require('lib.Camera')
        local camera = Camera.new("perspective", {
            position = {0, 0, 0},
            look_at = {0, 0, 1},
            up = {0, 1, 0},
            aspect_ratio = 1.0,
            fov = 90.0
        })
        
        local app_data = {
            get_string = function(self, key)
                if key == "camera_state" then
                    return '{"position":[10,20,30], "fov":45.0}'
                end
                return nil
            end
        }
        
        _G._thread_id = 1
        
        camera = CameraUtils.setup_or_sync_camera(camera, app_data, nil)
        
        _G._thread_id = nil
        
        return camera.position[1], camera.position[2], camera.position[3], camera.fov
    )";
    auto result = lua.safe_script(script);
    ASSERT_TRUE(result.valid()) << ((sol::error)result).what();
    auto [px, py, pz, fov] = result.get<std::tuple<double, double, double, double>>();
    ASSERT_EQ(px, 10.0);
    ASSERT_EQ(py, 20.0);
    ASSERT_EQ(pz, 30.0);
    ASSERT_EQ(fov, 45.0);
}
