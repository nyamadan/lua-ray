// test/vec3_test.cpp
// Vec3モジュールのユニットテスト（t-wada TDD: Red Phase）

#include <gtest/gtest.h>
#include <sol/sol.hpp>
#include <tuple>
#include <cmath>

class Vec3Test : public ::testing::Test {
protected:
    void SetUp() override {
        lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::math, sol::lib::table);
        // lib ディレクトリを package.path に追加
        lua.script("package.path = package.path .. ';./lib/?.lua;../../?.lua'");
    }

    sol::state lua;
};

// ===========================================
// 基本テスト: モジュール読み込み
// ===========================================

TEST_F(Vec3Test, CanRequireVec3) {
    auto result = lua.safe_script("return require('lib.Vec3')");
    ASSERT_TRUE(result.valid()) << "Vec3モジュールが読み込めません";
}

// ===========================================
// コンストラクタテスト
// ===========================================

TEST_F(Vec3Test, CreateVec3_Default) {
    auto result = lua.safe_script(R"(
        local Vec3 = require('lib.Vec3')
        local v = Vec3.new()
        return v.x, v.y, v.z
    )");
    ASSERT_TRUE(result.valid());
    auto [x, y, z] = result.get<std::tuple<double, double, double>>();
    ASSERT_EQ(x, 0.0);
    ASSERT_EQ(y, 0.0);
    ASSERT_EQ(z, 0.0);
}

TEST_F(Vec3Test, CreateVec3_WithValues) {
    auto result = lua.safe_script(R"(
        local Vec3 = require('lib.Vec3')
        local v = Vec3.new(1.0, 2.0, 3.0)
        return v.x, v.y, v.z
    )");
    ASSERT_TRUE(result.valid());
    auto [x, y, z] = result.get<std::tuple<double, double, double>>();
    ASSERT_EQ(x, 1.0);
    ASSERT_EQ(y, 2.0);
    ASSERT_EQ(z, 3.0);
}

// ===========================================
// 基本演算テスト
// ===========================================

TEST_F(Vec3Test, Add) {
    auto result = lua.safe_script(R"(
        local Vec3 = require('lib.Vec3')
        local a = Vec3.new(1, 2, 3)
        local b = Vec3.new(4, 5, 6)
        local c = a + b
        return c.x, c.y, c.z
    )");
    ASSERT_TRUE(result.valid());
    auto [x, y, z] = result.get<std::tuple<double, double, double>>();
    ASSERT_EQ(x, 5.0);
    ASSERT_EQ(y, 7.0);
    ASSERT_EQ(z, 9.0);
}

TEST_F(Vec3Test, Subtract) {
    auto result = lua.safe_script(R"(
        local Vec3 = require('lib.Vec3')
        local a = Vec3.new(4, 5, 6)
        local b = Vec3.new(1, 2, 3)
        local c = a - b
        return c.x, c.y, c.z
    )");
    ASSERT_TRUE(result.valid());
    auto [x, y, z] = result.get<std::tuple<double, double, double>>();
    ASSERT_EQ(x, 3.0);
    ASSERT_EQ(y, 3.0);
    ASSERT_EQ(z, 3.0);
}

TEST_F(Vec3Test, ScalarMultiply) {
    auto result = lua.safe_script(R"(
        local Vec3 = require('lib.Vec3')
        local a = Vec3.new(1, 2, 3)
        local b = a * 2
        return b.x, b.y, b.z
    )");
    ASSERT_TRUE(result.valid());
    auto [x, y, z] = result.get<std::tuple<double, double, double>>();
    ASSERT_EQ(x, 2.0);
    ASSERT_EQ(y, 4.0);
    ASSERT_EQ(z, 6.0);
}

TEST_F(Vec3Test, ScalarDivide) {
    auto result = lua.safe_script(R"(
        local Vec3 = require('lib.Vec3')
        local a = Vec3.new(2, 4, 6)
        local b = a / 2
        return b.x, b.y, b.z
    )");
    ASSERT_TRUE(result.valid());
    auto [x, y, z] = result.get<std::tuple<double, double, double>>();
    ASSERT_EQ(x, 1.0);
    ASSERT_EQ(y, 2.0);
    ASSERT_EQ(z, 3.0);
}

TEST_F(Vec3Test, UnaryMinus) {
    auto result = lua.safe_script(R"(
        local Vec3 = require('lib.Vec3')
        local a = Vec3.new(1, -2, 3)
        local b = -a
        return b.x, b.y, b.z
    )");
    ASSERT_TRUE(result.valid());
    auto [x, y, z] = result.get<std::tuple<double, double, double>>();
    ASSERT_EQ(x, -1.0);
    ASSERT_EQ(y, 2.0);
    ASSERT_EQ(z, -3.0);
}

// ===========================================
// ベクトル演算テスト
// ===========================================

TEST_F(Vec3Test, Dot) {
    auto result = lua.safe_script(R"(
        local Vec3 = require('lib.Vec3')
        local a = Vec3.new(1, 2, 3)
        local b = Vec3.new(4, 5, 6)
        return Vec3.dot(a, b)
    )");
    ASSERT_TRUE(result.valid());
    double dot = result.get<double>();
    ASSERT_EQ(dot, 32.0);  // 1*4 + 2*5 + 3*6 = 4 + 10 + 18 = 32
}

TEST_F(Vec3Test, Cross) {
    auto result = lua.safe_script(R"(
        local Vec3 = require('lib.Vec3')
        local a = Vec3.new(1, 0, 0)
        local b = Vec3.new(0, 1, 0)
        local c = Vec3.cross(a, b)
        return c.x, c.y, c.z
    )");
    ASSERT_TRUE(result.valid());
    auto [x, y, z] = result.get<std::tuple<double, double, double>>();
    ASSERT_EQ(x, 0.0);
    ASSERT_EQ(y, 0.0);
    ASSERT_EQ(z, 1.0);  // (1,0,0) × (0,1,0) = (0,0,1)
}

// ===========================================
// 長さ・正規化テスト
// ===========================================

TEST_F(Vec3Test, LengthSquared) {
    auto result = lua.safe_script(R"(
        local Vec3 = require('lib.Vec3')
        local v = Vec3.new(3, 4, 0)
        return v:length_squared()
    )");
    ASSERT_TRUE(result.valid());
    double len_sq = result.get<double>();
    ASSERT_EQ(len_sq, 25.0);  // 9 + 16 + 0 = 25
}

TEST_F(Vec3Test, Length) {
    auto result = lua.safe_script(R"(
        local Vec3 = require('lib.Vec3')
        local v = Vec3.new(3, 4, 0)
        return v:length()
    )");
    ASSERT_TRUE(result.valid());
    double len = result.get<double>();
    ASSERT_EQ(len, 5.0);
}

TEST_F(Vec3Test, Normalize) {
    auto result = lua.safe_script(R"(
        local Vec3 = require('lib.Vec3')
        local v = Vec3.new(3, 4, 0)
        local n = v:normalize()
        return n.x, n.y, n.z, n:length()
    )");
    ASSERT_TRUE(result.valid());
    auto [x, y, z, len] = result.get<std::tuple<double, double, double, double>>();
    ASSERT_NEAR(x, 0.6, 1e-9);
    ASSERT_NEAR(y, 0.8, 1e-9);
    ASSERT_NEAR(z, 0.0, 1e-9);
    ASSERT_NEAR(len, 1.0, 1e-9);
}

// ===========================================
// レイトレーシング用メソッドテスト
// ===========================================

TEST_F(Vec3Test, Reflect) {
    // 入射ベクトル v = (1, -1, 0) normalized と法線 n = (0, 1, 0) の反射
    // 反射ベクトル = v - 2*dot(v,n)*n = (1,-1,0) - 2*(-1)*(0,1,0) = (1,-1,0) + (0,2,0) = (1,1,0)
    auto result = lua.safe_script(R"(
        local Vec3 = require('lib.Vec3')
        local v = Vec3.new(1, -1, 0):normalize()
        local n = Vec3.new(0, 1, 0)
        local r = Vec3.reflect(v, n)
        return r.x, r.y, r.z
    )");
    ASSERT_TRUE(result.valid());
    auto [x, y, z] = result.get<std::tuple<double, double, double>>();
    double inv_sqrt2 = 1.0 / std::sqrt(2.0);
    ASSERT_NEAR(x, inv_sqrt2, 1e-9);
    ASSERT_NEAR(y, inv_sqrt2, 1e-9);
    ASSERT_NEAR(z, 0.0, 1e-9);
}

TEST_F(Vec3Test, NearZero) {
    auto result = lua.safe_script(R"(
        local Vec3 = require('lib.Vec3')
        local v1 = Vec3.new(1e-10, 1e-10, 1e-10)
        local v2 = Vec3.new(0.1, 0.0, 0.0)
        return v1:near_zero(), v2:near_zero()
    )");
    ASSERT_TRUE(result.valid());
    auto [near1, near2] = result.get<std::tuple<bool, bool>>();
    ASSERT_TRUE(near1);
    ASSERT_FALSE(near2);
}

// ===========================================
// ランダムベクトル生成テスト
// ===========================================

TEST_F(Vec3Test, RandomInUnitSphere) {
    auto result = lua.safe_script(R"(
        local Vec3 = require('lib.Vec3')
        local v = Vec3.random_in_unit_sphere()
        return v:length_squared()
    )");
    ASSERT_TRUE(result.valid());
    double len_sq = result.get<double>();
    ASSERT_LT(len_sq, 1.0);  // 単位球の内部なので長さは1未満
}

TEST_F(Vec3Test, RandomUnitVector) {
    auto result = lua.safe_script(R"(
        local Vec3 = require('lib.Vec3')
        local v = Vec3.random_unit_vector()
        return v:length()
    )");
    ASSERT_TRUE(result.valid());
    double len = result.get<double>();
    ASSERT_NEAR(len, 1.0, 1e-9);  // 単位ベクトルなので長さは1
}
