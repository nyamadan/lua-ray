// test/ray_test.cpp
// Rayモジュールのユニットテスト（t-wada TDD: Red Phase）

#include <gtest/gtest.h>
#include <sol/sol.hpp>
#include <tuple>
#include <cmath>

class RayTest : public ::testing::Test {
protected:
    void SetUp() override {
        lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::math, sol::lib::table);
        lua.script("package.path = package.path .. ';./lib/?.lua;../../?.lua'");
    }

    sol::state lua;
};

// ===========================================
// 基本テスト: モジュール読み込み
// ===========================================

TEST_F(RayTest, CanRequireRay) {
    auto result = lua.safe_script("return require('lib.Ray')");
    ASSERT_TRUE(result.valid()) << "Rayモジュールが読み込めません";
}

// ===========================================
// コンストラクタテスト
// ===========================================

TEST_F(RayTest, CreateRay) {
    auto result = lua.safe_script(R"(
        local Vec3 = require('lib.Vec3')
        local Ray = require('lib.Ray')
        local origin = Vec3.new(1, 2, 3)
        local direction = Vec3.new(0, 0, -1)
        local ray = Ray.new(origin, direction)
        return ray.origin.x, ray.origin.y, ray.origin.z,
               ray.direction.x, ray.direction.y, ray.direction.z
    )");
    ASSERT_TRUE(result.valid()) << ((sol::error)result).what();
    auto [ox, oy, oz, dx, dy, dz] = result.get<std::tuple<double, double, double, double, double, double>>();
    ASSERT_EQ(ox, 1.0);
    ASSERT_EQ(oy, 2.0);
    ASSERT_EQ(oz, 3.0);
    ASSERT_EQ(dx, 0.0);
    ASSERT_EQ(dy, 0.0);
    ASSERT_EQ(dz, -1.0);
}

// ===========================================
// at(t) メソッドテスト
// ===========================================

TEST_F(RayTest, At_t0) {
    auto result = lua.safe_script(R"(
        local Vec3 = require('lib.Vec3')
        local Ray = require('lib.Ray')
        local origin = Vec3.new(0, 0, 0)
        local direction = Vec3.new(1, 0, 0)
        local ray = Ray.new(origin, direction)
        local p = ray:at(0)
        return p.x, p.y, p.z
    )");
    ASSERT_TRUE(result.valid()) << ((sol::error)result).what();
    auto [x, y, z] = result.get<std::tuple<double, double, double>>();
    ASSERT_EQ(x, 0.0);
    ASSERT_EQ(y, 0.0);
    ASSERT_EQ(z, 0.0);
}

TEST_F(RayTest, At_t1) {
    auto result = lua.safe_script(R"(
        local Vec3 = require('lib.Vec3')
        local Ray = require('lib.Ray')
        local origin = Vec3.new(0, 0, 0)
        local direction = Vec3.new(1, 0, 0)
        local ray = Ray.new(origin, direction)
        local p = ray:at(1)
        return p.x, p.y, p.z
    )");
    ASSERT_TRUE(result.valid()) << ((sol::error)result).what();
    auto [x, y, z] = result.get<std::tuple<double, double, double>>();
    ASSERT_EQ(x, 1.0);
    ASSERT_EQ(y, 0.0);
    ASSERT_EQ(z, 0.0);
}

TEST_F(RayTest, At_ArbitraryT) {
    auto result = lua.safe_script(R"(
        local Vec3 = require('lib.Vec3')
        local Ray = require('lib.Ray')
        local origin = Vec3.new(1, 2, 3)
        local direction = Vec3.new(1, 2, 3)
        local ray = Ray.new(origin, direction)
        local p = ray:at(2)
        return p.x, p.y, p.z
    )");
    ASSERT_TRUE(result.valid()) << ((sol::error)result).what();
    auto [x, y, z] = result.get<std::tuple<double, double, double>>();
    // origin + t * direction = (1,2,3) + 2 * (1,2,3) = (3, 6, 9)
    ASSERT_EQ(x, 3.0);
    ASSERT_EQ(y, 6.0);
    ASSERT_EQ(z, 9.0);
}

// ===========================================
// 負のtの処理
// ===========================================

TEST_F(RayTest, At_NegativeT) {
    auto result = lua.safe_script(R"(
        local Vec3 = require('lib.Vec3')
        local Ray = require('lib.Ray')
        local origin = Vec3.new(0, 0, 0)
        local direction = Vec3.new(1, 0, 0)
        local ray = Ray.new(origin, direction)
        local p = ray:at(-1)
        return p.x, p.y, p.z
    )");
    ASSERT_TRUE(result.valid()) << ((sol::error)result).what();
    auto [x, y, z] = result.get<std::tuple<double, double, double>>();
    ASSERT_EQ(x, -1.0);
    ASSERT_EQ(y, 0.0);
    ASSERT_EQ(z, 0.0);
}
