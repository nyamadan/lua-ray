// test/pathtracer_test.cpp
// PathTracerモジュールのユニットテスト（t-wada TDD: Red Phase）

#include <gtest/gtest.h>
#include <sol/sol.hpp>
#include <tuple>
#include <cmath>

class PathTracerTest : public ::testing::Test {
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

TEST_F(PathTracerTest, CanRequirePathTracer) {
    auto result = lua.safe_script("return require('lib.PathTracer')");
    ASSERT_TRUE(result.valid()) << "PathTracerモジュールが読み込めません";
}

// ===========================================
// 正規直交基底生成テスト
// ===========================================

TEST_F(PathTracerTest, CreateOrthonormalBasis) {
    auto result = lua.safe_script(R"(
        local Vec3 = require('lib.Vec3')
        local PathTracer = require('lib.PathTracer')
        
        -- 法線が上向きの場合
        local normal = Vec3.new(0, 1, 0)
        local w, u, v = PathTracer.create_orthonormal_basis(normal)
        
        -- wは法線と同じ
        local w_is_normal = math.abs(w.x - normal.x) < 0.0001 and
                            math.abs(w.y - normal.y) < 0.0001 and
                            math.abs(w.z - normal.z) < 0.0001
        
        -- u, vはwに直交
        local u_dot_w = Vec3.dot(u, w)
        local v_dot_w = Vec3.dot(v, w)
        local u_dot_v = Vec3.dot(u, v)
        
        return w_is_normal, 
               math.abs(u_dot_w) < 0.0001, 
               math.abs(v_dot_w) < 0.0001, 
               math.abs(u_dot_v) < 0.0001
    )");
    ASSERT_TRUE(result.valid()) << ((sol::error)result).what();
    auto [w_is_normal, u_perp_w, v_perp_w, u_perp_v] = result.get<std::tuple<bool, bool, bool, bool>>();
    ASSERT_TRUE(w_is_normal);
    ASSERT_TRUE(u_perp_w);
    ASSERT_TRUE(v_perp_w);
    ASSERT_TRUE(u_perp_v);
}

// ===========================================
// コサイン重点サンプリングテスト
// ===========================================

TEST_F(PathTracerTest, CosineWeightedSampleOnHemisphere) {
    auto result = lua.safe_script(R"(
        local Vec3 = require('lib.Vec3')
        local PathTracer = require('lib.PathTracer')
        
        -- 法線が上向きの場合
        local normal = Vec3.new(0, 1, 0)
        
        -- サンプリング
        local dir = PathTracer.cosine_weighted_sample(normal)
        
        -- 方向は上半球内
        local dot_with_normal = Vec3.dot(dir, normal)
        
        -- 正規化されている
        local is_normalized = math.abs(dir:length() - 1.0) < 0.0001
        
        return dot_with_normal >= 0, is_normalized
    )");
    ASSERT_TRUE(result.valid()) << ((sol::error)result).what();
    auto [in_hemisphere, is_normalized] = result.get<std::tuple<bool, bool>>();
    ASSERT_TRUE(in_hemisphere);
    ASSERT_TRUE(is_normalized);
}

// ===========================================
// radiance関数の基本テスト
// ===========================================

TEST_F(PathTracerTest, RadianceReturnsBlackForNoHit) {
    // シーンに何もない場合、背景色（黒）を返す
    auto result = lua.safe_script(R"(
        local Vec3 = require('lib.Vec3')
        local Ray = require('lib.Ray')
        local PathTracer = require('lib.PathTracer')
        
        -- モックシーン（常にヒットしない）
        local mock_scene = {
            intersect = function(self, ox, oy, oz, dx, dy, dz)
                return false, 0, 0, 0, 0, -1, -1
            end
        }
        
        local materials = {}
        local ray = Ray.new(Vec3.new(0, 0, 0), Vec3.new(0, 0, 1))
        
        local color = PathTracer.radiance(ray, mock_scene, materials, 5)
        
        -- 背景は黒
        return color.x, color.y, color.z
    )");
    ASSERT_TRUE(result.valid()) << ((sol::error)result).what();
    auto [x, y, z] = result.get<std::tuple<double, double, double>>();
    ASSERT_EQ(x, 0.0);
    ASSERT_EQ(y, 0.0);
    ASSERT_EQ(z, 0.0);
}

TEST_F(PathTracerTest, RadianceReturnsEmissionForLight) {
    // 発光体にヒットした場合、その発光色を返す
    auto result = lua.safe_script(R"(
        local Vec3 = require('lib.Vec3')
        local Ray = require('lib.Ray')
        local Material = require('lib.Material')
        local PathTracer = require('lib.PathTracer')
        
        -- モックシーン（常にヒットする）
        local mock_scene = {
            intersect = function(self, ox, oy, oz, dx, dy, dz)
                -- hit, t, nx, ny, nz, geomID, primID
                return true, 1.0, 0, 1, 0, 0, 0
            end
        }
        
        -- 発光マテリアル
        local materials = {
            [0] = Material.DiffuseLight(Vec3.new(10.0, 10.0, 10.0))
        }
        
        local ray = Ray.new(Vec3.new(0, 2, 0), Vec3.new(0, -1, 0))
        
        local color = PathTracer.radiance(ray, mock_scene, materials, 5)
        
        return color.x, color.y, color.z
    )");
    ASSERT_TRUE(result.valid()) << ((sol::error)result).what();
    auto [x, y, z] = result.get<std::tuple<double, double, double>>();
    ASSERT_EQ(x, 10.0);
    ASSERT_EQ(y, 10.0);
    ASSERT_EQ(z, 10.0);
}

TEST_F(PathTracerTest, RadianceReturnsZeroAtMaxDepth) {
    // 深度0の場合、即座に黒を返す
    auto result = lua.safe_script(R"(
        local Vec3 = require('lib.Vec3')
        local Ray = require('lib.Ray')
        local PathTracer = require('lib.PathTracer')
        
        local mock_scene = {
            intersect = function(self, ox, oy, oz, dx, dy, dz)
                return true, 1.0, 0, 1, 0, 0, 0
            end
        }
        
        local materials = {}
        local ray = Ray.new(Vec3.new(0, 0, 0), Vec3.new(0, 0, 1))
        
        -- depth = 0
        local color = PathTracer.radiance(ray, mock_scene, materials, 0)
        
        return color.x, color.y, color.z
    )");
    ASSERT_TRUE(result.valid()) << ((sol::error)result).what();
    auto [x, y, z] = result.get<std::tuple<double, double, double>>();
    ASSERT_EQ(x, 0.0);
    ASSERT_EQ(y, 0.0);
    ASSERT_EQ(z, 0.0);
}
