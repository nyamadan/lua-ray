#include <gtest/gtest.h>
#include <sol/sol.hpp>
#include <tuple>

class CameraTest : public ::testing::Test {
protected:
    void SetUp() override {
        lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::math, sol::lib::table);
        // Add lib directory to package.path
        // Assuming running from build directory and accessing source lib via ../../lib or similar
        lua.script("package.path = package.path .. ';./lib/?.lua;../../?.lua'");
    }

    sol::state lua;
};

TEST_F(CameraTest, CanRequireCamera) {
    auto result = lua.safe_script("return require('lib.Camera')");
    ASSERT_TRUE(result.valid());
}

TEST_F(CameraTest, CreatePerspectiveCamera) {
    auto script = R"(
        local Camera = require('lib.Camera')
        local c = Camera.new('perspective', {
            position = {0, 0, 5},
            look_at = {0, 0, 0},
            up = {0, 1, 0},
            fov = 60,
            aspect_ratio = 1.33
        })
        return c.camera_type, c.position[3], c.fov
    )";
    auto result = lua.safe_script(script);
    ASSERT_TRUE(result.valid());
    std::string type;
    double z, fov;
    std::tie(type, z, fov) = result.get<std::tuple<std::string, double, double>>();
    ASSERT_EQ(type, "perspective");
    ASSERT_EQ(z, 5.0);
    ASSERT_EQ(fov, 60.0);
}

TEST_F(CameraTest, ComputeBasis_Aligned) {
    // Case 1: Camera at (0,0,1) looking at (0,0,0) with up (0,1,0)
    // Forward = (0,0,-1), Right = (1,0,0), Up = (0,1,0)
    auto script = R"(
        local Camera = require('lib.Camera')
        local c = Camera.new('perspective', {
            position = {0, 0, 1},
            look_at = {0, 0, 0},
            up = {0, 1, 0}
        })
        return c.forward, c.right, c.camera_up
    )";
    auto result = lua.safe_script(script);
    ASSERT_TRUE(result.valid());
    auto [fwd, right, up] = result.get<std::tuple<sol::table, sol::table, sol::table>>();
    
    // Check Forward (0,0,-1)
    ASSERT_NEAR(fwd[1], 0.0, 1e-5);
    ASSERT_NEAR(fwd[2], 0.0, 1e-5);
    ASSERT_NEAR(fwd[3], -1.0, 1e-5);

    // Check Right (1,0,0)
    ASSERT_NEAR(right[1], 1.0, 1e-5);
    ASSERT_NEAR(right[2], 0.0, 1e-5);
    ASSERT_NEAR(right[3], 0.0, 1e-5);

    // Check Up (0,1,0)
    ASSERT_NEAR(up[1], 0.0, 1e-5);
    ASSERT_NEAR(up[2], 1.0, 1e-5);
    ASSERT_NEAR(up[3], 0.0, 1e-5);
}

TEST_F(CameraTest, GenerateRay_Center) {
    // Center of screen (u=0, v=0) should generate ray along Forward vector
    auto script = R"(
        local Camera = require('lib.Camera')
        local c = Camera.new('perspective', {
            position = {0, 0, 1},
            look_at = {0, 0, 0},
            up = {0, 1, 0},
            fov = 90,
            aspect_ratio = 1.0
        })
        -- u=0, v=0 maps to center
        local ox, oy, oz, dx, dy, dz = c:generate_ray(0, 0)
        return ox, oy, oz, dx, dy, dz
    )";
    auto result = lua.safe_script(script);
    ASSERT_TRUE(result.valid());
    auto [ox, oy, oz, dx, dy, dz] = result.get<std::tuple<double, double, double, double, double, double>>();
    
    // Origin should be camera position
    ASSERT_EQ(ox, 0.0);
    ASSERT_EQ(oy, 0.0);
    ASSERT_EQ(oz, 1.0);
    
    // Direction should be (0,0,-1)
    ASSERT_NEAR(dx, 0.0, 1e-5);
    ASSERT_NEAR(dy, 0.0, 1e-5);
    ASSERT_NEAR(dz, -1.0, 1e-5);
}

TEST_F(CameraTest, MoveForward) {
    // Camera at (0,0,1) looking at (0,0,0). Forward=(0,0,-1).
    // move_forward(0.5) should move both position and look_at by 0.5 * forward.
    auto script = R"(
        local Camera = require('lib.Camera')
        local c = Camera.new('perspective', {
            position = {0, 0, 1},
            look_at = {0, 0, 0},
            up = {0, 1, 0},
            fov = 60,
            aspect_ratio = 1.0
        })
        c:move_forward(0.5)
        return c.position[1], c.position[2], c.position[3],
               c.look_at[1], c.look_at[2], c.look_at[3]
    )";
    auto result = lua.safe_script(script);
    ASSERT_TRUE(result.valid()) << ((sol::error)result).what();
    auto [px, py, pz, lx, ly, lz] = result.get<std::tuple<double, double, double, double, double, double>>();
    
    ASSERT_NEAR(px, 0.0, 1e-5);
    ASSERT_NEAR(py, 0.0, 1e-5);
    ASSERT_NEAR(pz, 0.5, 1e-5);
    
    ASSERT_NEAR(lx, 0.0, 1e-5);
    ASSERT_NEAR(ly, 0.0, 1e-5);
    ASSERT_NEAR(lz, -0.5, 1e-5);
}

TEST_F(CameraTest, MoveRight) {
    // Camera at (0,0,1) looking at (0,0,0). Right=(1,0,0).
    auto script = R"(
        local Camera = require('lib.Camera')
        local c = Camera.new('perspective', {
            position = {0, 0, 1},
            look_at = {0, 0, 0},
            up = {0, 1, 0},
            fov = 60,
            aspect_ratio = 1.0
        })
        c:move_right(1.0)
        return c.position[1], c.position[2], c.position[3],
               c.look_at[1], c.look_at[2], c.look_at[3]
    )";
    auto result = lua.safe_script(script);
    ASSERT_TRUE(result.valid()) << ((sol::error)result).what();
    auto [px, py, pz, lx, ly, lz] = result.get<std::tuple<double, double, double, double, double, double>>();
    
    ASSERT_NEAR(px, 1.0, 1e-5);
    ASSERT_NEAR(py, 0.0, 1e-5);
    ASSERT_NEAR(pz, 1.0, 1e-5);
    
    ASSERT_NEAR(lx, 1.0, 1e-5);
    ASSERT_NEAR(ly, 0.0, 1e-5);
    ASSERT_NEAR(lz, 0.0, 1e-5);
}

TEST_F(CameraTest, MoveUp) {
    // Camera at (0,0,1) looking at (0,0,0). camera_up=(0,1,0).
    auto script = R"(
        local Camera = require('lib.Camera')
        local c = Camera.new('perspective', {
            position = {0, 0, 1},
            look_at = {0, 0, 0},
            up = {0, 1, 0},
            fov = 60,
            aspect_ratio = 1.0
        })
        c:move_up(2.0)
        return c.position[1], c.position[2], c.position[3],
               c.look_at[1], c.look_at[2], c.look_at[3]
    )";
    auto result = lua.safe_script(script);
    ASSERT_TRUE(result.valid()) << ((sol::error)result).what();
    auto [px, py, pz, lx, ly, lz] = result.get<std::tuple<double, double, double, double, double, double>>();
    
    ASSERT_NEAR(px, 0.0, 1e-5);
    ASSERT_NEAR(py, 2.0, 1e-5);
    ASSERT_NEAR(pz, 1.0, 1e-5);
    
    ASSERT_NEAR(lx, 0.0, 1e-5);
    ASSERT_NEAR(ly, 2.0, 1e-5);
    ASSERT_NEAR(lz, 0.0, 1e-5);
}
TEST_F(CameraTest, GenerateRay_TopRight) {
    // u=1, v=1 -> Top Right corner
    // tan(45) = 1. half_height = 1.
    // aspect = 1. half_width = 1.
    // screen point = (1, 1) relative to center on image plane z=-1?
    // Wait, implementation: dx = forward + screen_x*right + screen_y*up
    // forward=(0,0,-1). right=(1,0,0). up=(0,1,0).
    // dx_vec = (0,0,-1) + 1*(1,0,0) + 1*(0,1,0) = (1, 1, -1)
    // Normalized: (1/sqrt(3), 1/sqrt(3), -1/sqrt(3))
    
    auto script = R"(
        local Camera = require('lib.Camera')
        local c = Camera.new('perspective', {
            position = {0, 0, 1},
            look_at = {0, 0, 0},
            up = {0, 1, 0},
            fov = 90,
            aspect_ratio = 1.0
        })
        local ox, oy, oz, dx, dy, dz = c:generate_ray(1, 1)
        return dx, dy, dz
    )";
    auto result = lua.safe_script(script);
    ASSERT_TRUE(result.valid());
    auto [dx, dy, dz] = result.get<std::tuple<double, double, double>>();
    
    double val = 1.0 / std::sqrt(3.0);
    ASSERT_NEAR(dx, val, 1e-5);
    ASSERT_NEAR(dy, val, 1e-5);
    ASSERT_NEAR(dz, -val, 1e-5);
}

TEST_F(CameraTest, CreateOrthographicCamera) {
    auto script = R"(
        local Camera = require('lib.Camera')
        local c = Camera.new('orthographic', {
            position = {0, 0, 10},
            look_at = {0, 0, 0},
            up = {0, 1, 0},
            ortho_height = 5.0,
            aspect_ratio = 1.5
        })
        return c.camera_type, c.ortho_height
    )";
    auto result = lua.safe_script(script);
    ASSERT_TRUE(result.valid());
    std::string type;
    double height;
    std::tie(type, height) = result.get<std::tuple<std::string, double>>();
    ASSERT_EQ(type, "orthographic");
    ASSERT_EQ(height, 5.0);
}

TEST_F(CameraTest, GenerateOrthographicRay) {
    // Camera at (0,0,10) looking at (0,0,0). Forward=(0,0,-1).
    // Right=(1,0,0), Up=(0,1,0).
    // Ortho height=2. Aspect=1. half_height=1. half_width=1.
    // u=0.5, v=0.5
    // screen_x = 0.5 * 1 = 0.5
    // screen_y = 0.5 * 1 = 0.5
    // Origin = Pos + 0.5*Right + 0.5*Up = (0.5, 0.5, 10)
    // Direction = Forward = (0, 0, -1)
    
    auto script = R"(
        local Camera = require('lib.Camera')
        local c = Camera.new('orthographic', {
            position = {0, 0, 10},
            look_at = {0, 0, 0},
            up = {0, 1, 0},
            ortho_height = 2.0,
            aspect_ratio = 1.0
        })
        local ox, oy, oz, dx, dy, dz = c:generate_ray(0.5, 0.5)
        return ox, oy, oz, dx, dy, dz
    )";
    auto result = lua.safe_script(script);
    ASSERT_TRUE(result.valid());
    auto [ox, oy, oz, dx, dy, dz] = result.get<std::tuple<double, double, double, double, double, double>>();
    
    ASSERT_NEAR(ox, 0.5, 1e-5);
    ASSERT_NEAR(oy, 0.5, 1e-5);
    ASSERT_NEAR(oz, 10.0, 1e-5);
    
    ASSERT_NEAR(dx, 0.0, 1e-5);
    ASSERT_NEAR(dy, 0.0, 1e-5);
    ASSERT_NEAR(dz, -1.0, 1e-5);
}

TEST_F(CameraTest, RotateCamera_YawAndPitch) {
    // Camera at (0,0,0) looking at (0,0,1). Forward=(0,0,1).
    // Rotate by 90 degrees yaw (around Y axis). Should look at (1,0,0).
    auto script = R"(
        local Camera = require('lib.Camera')
        local c = Camera.new('perspective', {
            position = {0, 0, 0},
            look_at = {0, 0, 1},
            up = {0, 1, 0},
            fov = 60,
            aspect_ratio = 1.0
        })
        -- delta_yaw (degrees), delta_pitch (degrees)
        c:rotate(90.0, 0.0)
        
        -- Forward needs to be close to (1, 0, 0)
        -- since it looks from 0,0,0 to 1,0,0.
        return c.forward[1], c.forward[2], c.forward[3],
               c.look_at[1], c.look_at[2], c.look_at[3]
    )";
    auto result = lua.safe_script(script);
    ASSERT_TRUE(result.valid()) << ((sol::error)result).what();
    auto [fx, fy, fz, lx, ly, lz] = result.get<std::tuple<double, double, double, double, double, double>>();
    
    ASSERT_NEAR(fx, 1.0, 1e-5);
    ASSERT_NEAR(fy, 0.0, 1e-5);
    ASSERT_NEAR(fz, 0.0, 1e-5);
    
    // look_at point is defined exactly as position + forward
    ASSERT_NEAR(lx, 1.0, 1e-5);
    ASSERT_NEAR(ly, 0.0, 1e-5);
    ASSERT_NEAR(lz, 0.0, 1e-5);
}

