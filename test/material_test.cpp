// test/material_test.cpp
// Materialモジュールのユニットテスト（t-wada TDD: Red Phase）

#include <gtest/gtest.h>
#include <sol/sol.hpp>
#include <tuple>
#include <cmath>

class MaterialTest : public ::testing::Test {
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

TEST_F(MaterialTest, CanRequireMaterial) {
    auto result = lua.safe_script("return require('lib.Material')");
    ASSERT_TRUE(result.valid()) << "Materialモジュールが読み込めません";
}

// ===========================================
// Lambertian (拡散反射) テスト
// ===========================================

TEST_F(MaterialTest, CreateLambertian) {
    auto result = lua.safe_script(R"(
        local Vec3 = require('lib.Vec3')
        local Material = require('lib.Material')
        local albedo = Vec3.new(0.5, 0.5, 0.5)
        local mat = Material.Lambertian(albedo)
        return mat.type, mat.albedo.x, mat.albedo.y, mat.albedo.z
    )");
    ASSERT_TRUE(result.valid()) << ((sol::error)result).what();
    auto [type, x, y, z] = result.get<std::tuple<std::string, double, double, double>>();
    ASSERT_EQ(type, "lambertian");
    ASSERT_EQ(x, 0.5);
    ASSERT_EQ(y, 0.5);
    ASSERT_EQ(z, 0.5);
}

TEST_F(MaterialTest, LambertianScatterReturnsRay) {
    auto result = lua.safe_script(R"(
        local Vec3 = require('lib.Vec3')
        local Ray = require('lib.Ray')
        local Material = require('lib.Material')
        
        local albedo = Vec3.new(0.5, 0.5, 0.5)
        local mat = Material.Lambertian(albedo)
        
        -- ヒット情報を模倣
        local hit_record = {
            p = Vec3.new(0, 0, 0),
            normal = Vec3.new(0, 1, 0),
            t = 1.0,
            front_face = true
        }
        
        local ray_in = Ray.new(Vec3.new(0, 1, 0), Vec3.new(0, -1, 0))
        local scattered, attenuation = mat:scatter(ray_in, hit_record)
        
        -- 散乱レイが存在すること
        return scattered ~= nil, attenuation ~= nil
    )");
    ASSERT_TRUE(result.valid()) << ((sol::error)result).what();
    auto [has_ray, has_attenuation] = result.get<std::tuple<bool, bool>>();
    ASSERT_TRUE(has_ray);
    ASSERT_TRUE(has_attenuation);
}

// ===========================================
// Metal (鏡面反射) テスト
// ===========================================

TEST_F(MaterialTest, CreateMetal) {
    auto result = lua.safe_script(R"(
        local Vec3 = require('lib.Vec3')
        local Material = require('lib.Material')
        local albedo = Vec3.new(0.8, 0.8, 0.8)
        local mat = Material.Metal(albedo, 0.3)
        return mat.type, mat.albedo.x, mat.fuzz
    )");
    ASSERT_TRUE(result.valid()) << ((sol::error)result).what();
    auto [type, x, fuzz] = result.get<std::tuple<std::string, double, double>>();
    ASSERT_EQ(type, "metal");
    ASSERT_EQ(x, 0.8);
    ASSERT_EQ(fuzz, 0.3);
}

TEST_F(MaterialTest, MetalFuzzClampedTo1) {
    auto result = lua.safe_script(R"(
        local Vec3 = require('lib.Vec3')
        local Material = require('lib.Material')
        local albedo = Vec3.new(0.8, 0.8, 0.8)
        local mat = Material.Metal(albedo, 1.5)
        return mat.fuzz
    )");
    ASSERT_TRUE(result.valid()) << ((sol::error)result).what();
    double fuzz = result.get<double>();
    ASSERT_EQ(fuzz, 1.0); // 1.5は1.0にクランプされる
}

TEST_F(MaterialTest, MetalScatterReflects) {
    auto result = lua.safe_script(R"(
        local Vec3 = require('lib.Vec3')
        local Ray = require('lib.Ray')
        local Material = require('lib.Material')
        
        local albedo = Vec3.new(1.0, 1.0, 1.0)
        local mat = Material.Metal(albedo, 0.0) -- fuzz = 0 で完全反射
        
        local hit_record = {
            p = Vec3.new(0, 0, 0),
            normal = Vec3.new(0, 1, 0),
            t = 1.0,
            front_face = true
        }
        
        local ray_in = Ray.new(Vec3.new(-1, 1, 0), Vec3.new(1, -1, 0):normalize())
        local scattered, attenuation = mat:scatter(ray_in, hit_record)
        
        -- 反射レイの方向を確認
        if scattered then
            local dir = scattered.direction
            -- Y成分が正（上向きに反射）
            return dir.y > 0
        end
        return false
    )");
    ASSERT_TRUE(result.valid()) << ((sol::error)result).what();
    bool reflected_upward = result.get<bool>();
    ASSERT_TRUE(reflected_upward);
}

// ===========================================
// Dielectric (誘電体/ガラス) テスト
// ===========================================

TEST_F(MaterialTest, CreateDielectric) {
    auto result = lua.safe_script(R"(
        local Vec3 = require('lib.Vec3')
        local Material = require('lib.Material')
        local mat = Material.Dielectric(1.5)
        return mat.type, mat.ir
    )");
    ASSERT_TRUE(result.valid()) << ((sol::error)result).what();
    auto [type, ir] = result.get<std::tuple<std::string, double>>();
    ASSERT_EQ(type, "dielectric");
    ASSERT_EQ(ir, 1.5);
}

TEST_F(MaterialTest, DielectricScatterReturnsRay) {
    auto result = lua.safe_script(R"(
        local Vec3 = require('lib.Vec3')
        local Ray = require('lib.Ray')
        local Material = require('lib.Material')
        
        local mat = Material.Dielectric(1.5)
        
        local hit_record = {
            p = Vec3.new(0, 0, 0),
            normal = Vec3.new(0, 1, 0),
            t = 1.0,
            front_face = true
        }
        
        local ray_in = Ray.new(Vec3.new(0, 1, 0), Vec3.new(0, -1, 0))
        local scattered, attenuation = mat:scatter(ray_in, hit_record)
        
        return scattered ~= nil, attenuation ~= nil
    )");
    ASSERT_TRUE(result.valid()) << ((sol::error)result).what();
    auto [has_ray, has_attenuation] = result.get<std::tuple<bool, bool>>();
    ASSERT_TRUE(has_ray);
    ASSERT_TRUE(has_attenuation);
}

TEST_F(MaterialTest, DielectricAttenuationIsWhite) {
    auto result = lua.safe_script(R"(
        local Vec3 = require('lib.Vec3')
        local Ray = require('lib.Ray')
        local Material = require('lib.Material')
        
        local mat = Material.Dielectric(1.5)
        
        local hit_record = {
            p = Vec3.new(0, 0, 0),
            normal = Vec3.new(0, 1, 0),
            t = 1.0,
            front_face = true
        }
        
        local ray_in = Ray.new(Vec3.new(0, 1, 0), Vec3.new(0, -1, 0))
        local scattered, attenuation = mat:scatter(ray_in, hit_record)
        
        -- ガラスの減衰は白（1,1,1）
        return attenuation.x, attenuation.y, attenuation.z
    )");
    ASSERT_TRUE(result.valid()) << ((sol::error)result).what();
    auto [x, y, z] = result.get<std::tuple<double, double, double>>();
    ASSERT_EQ(x, 1.0);
    ASSERT_EQ(y, 1.0);
    ASSERT_EQ(z, 1.0);
}
