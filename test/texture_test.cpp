#include <gtest/gtest.h>
#include "lua_binding.h"
#include "embree_wrapper.h"
#include "gltf_loader.h"

// =============================================================
// テストリスト (TDD):
// 1. [ ] Texture.new でテクスチャオブジェクトを生成できる
// 2. [ ] texture:sample(u, v) で正しい色を返す
// 3. [ ] Texture.interpolate_uv でバリセントリック座標からUVを補間できる
// 4. [ ] テクスチャのラップ（u,v > 1.0 や < 0.0）が正しく処理される
// 5. [ ] BoxTextured.glb のデータでテクスチャ生成・サンプリングできる
// =============================================================

class TextureTest : public ::testing::Test {
protected:
    sol::state lua;

    void SetUp() override {
        lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::math,
                          sol::lib::string, sol::lib::table);
        bind_common_types(lua);

        // Luaパッケージパスを設定
        lua["package"]["path"] = "?.lua;?/init.lua;lib/?.lua";
    }
};

// --- テスト1: Texture.new でテクスチャオブジェクトを生成できる ---
TEST_F(TextureTest, CanCreateTexture) {
    auto result = lua.safe_script(R"(
        local Texture = require('lib.Texture')

        -- 2x2 RGB テクスチャ（赤、緑、青、白）
        local pixels = {
            255, 0, 0,    -- (0,0) 赤
            0, 255, 0,    -- (1,0) 緑
            0, 0, 255,    -- (0,1) 青
            255, 255, 255 -- (1,1) 白
        }
        local tex = Texture.new(2, 2, 3, pixels)
        assert(tex ~= nil, "テクスチャ生成に失敗")
        assert(tex.width == 2, "幅が2でない")
        assert(tex.height == 2, "高さが2でない")
        return true
    )", sol::script_pass_on_error);
    ASSERT_TRUE(result.valid()) << "Lua error: " << sol::error(result).what();
    EXPECT_TRUE(result.get<bool>());
}

// --- テスト2: texture:sample(u, v) で正しい色を返す ---
TEST_F(TextureTest, SampleReturnsCorrectColor) {
    auto result = lua.safe_script(R"(
        local Texture = require('lib.Texture')

        -- 2x2 RGB テクスチャ（赤、緑、青、白）
        local pixels = {
            255, 0, 0,    -- (0,0) 赤
            0, 255, 0,    -- (1,0) 緑
            0, 0, 255,    -- (0,1) 青
            255, 255, 255 -- (1,1) 白
        }
        local tex = Texture.new(2, 2, 3, pixels)

        -- 左上 (0, 0) → 赤
        local r, g, b = tex:sample(0.0, 0.0)
        assert(r == 255 and g == 0 and b == 0, "左上が赤でない: " .. r .. "," .. g .. "," .. b)

        -- 右上 (1, 0) → 緑（最近傍サンプリング）
        local r2, g2, b2 = tex:sample(0.99, 0.0)
        assert(r2 == 0 and g2 == 255 and b2 == 0, "右上が緑でない: " .. r2 .. "," .. g2 .. "," .. b2)

        -- 左下 (0, 1) → 青
        local r3, g3, b3 = tex:sample(0.0, 0.99)
        assert(r3 == 0 and g3 == 0 and b3 == 255, "左下が青でない: " .. r3 .. "," .. g3 .. "," .. b3)

        return true
    )", sol::script_pass_on_error);
    ASSERT_TRUE(result.valid()) << "Lua error: " << sol::error(result).what();
    EXPECT_TRUE(result.get<bool>());
}

// --- テスト3: Texture.interpolate_uv でバリセントリック座標からUVを補間できる ---
TEST_F(TextureTest, InterpolateUV) {
    auto result = lua.safe_script(R"(
        local Texture = require('lib.Texture')

        -- テスト用 UV 座標（フラット配列、0-indexed頂点に対応）
        -- 頂点0: (0.0, 0.0), 頂点1: (1.0, 0.0), 頂点2: (0.0, 1.0)
        local texcoords = {0.0, 0.0, 1.0, 0.0, 0.0, 1.0}

        -- テスト用インデックス（0-indexed）
        -- 三角形0: 頂点0, 1, 2
        local indices = {0, 1, 2}

        -- primID=0, baryU=0.5, baryV=0.0 → 頂点0と1の中点
        -- UV = (1-u-v)*uv0 + u*uv1 + v*uv2 = 0.5*(0,0) + 0.5*(1,0) + 0*(0,1) = (0.5, 0)
        local u, v = Texture.interpolate_uv(texcoords, indices, 0, 0.5, 0.0)
        assert(math.abs(u - 0.5) < 0.001, "u が 0.5 でない: " .. u)
        assert(math.abs(v - 0.0) < 0.001, "v が 0.0 でない: " .. v)

        -- primID=0, baryU=0.0, baryV=0.5 → 頂点0と2の中点
        -- UV = 0.5*(0,0) + 0*(1,0) + 0.5*(0,1) = (0, 0.5)
        local u2, v2 = Texture.interpolate_uv(texcoords, indices, 0, 0.0, 0.5)
        assert(math.abs(u2 - 0.0) < 0.001, "u2 が 0.0 でない: " .. u2)
        assert(math.abs(v2 - 0.5) < 0.001, "v2 が 0.5 でない: " .. v2)

        return true
    )", sol::script_pass_on_error);
    ASSERT_TRUE(result.valid()) << "Lua error: " << sol::error(result).what();
    EXPECT_TRUE(result.get<bool>());
}

// --- テスト4: テクスチャのラップ処理 ---
TEST_F(TextureTest, TextureWrapping) {
    auto result = lua.safe_script(R"(
        local Texture = require('lib.Texture')

        -- 2x2 RGB テクスチャ
        local pixels = {
            255, 0, 0,    -- (0,0) 赤
            0, 255, 0,    -- (1,0) 緑
            0, 0, 255,    -- (0,1) 青
            255, 255, 255 -- (1,1) 白
        }
        local tex = Texture.new(2, 2, 3, pixels)

        -- u=1.5 → wrap → u=0.5 → 右列、v=0 → 上段 → 緑
        local r, g, b = tex:sample(1.5, 0.0)
        assert(r == 0 and g == 255 and b == 0, "ラップ処理が不正: " .. r .. "," .. g .. "," .. b)

        -- u=-0.5 → wrap → u=0.5 → 右列
        local r2, g2, b2 = tex:sample(-0.5, 0.0)
        assert(r2 == 0 and g2 == 255 and b2 == 0, "負のラップ処理が不正: " .. r2 .. "," .. g2 .. "," .. b2)

        return true
    )", sol::script_pass_on_error);
    ASSERT_TRUE(result.valid()) << "Lua error: " << sol::error(result).what();
    EXPECT_TRUE(result.get<bool>());
}

// --- テスト5: BoxTextured.glb のデータでテクスチャ生成・サンプリング ---
TEST_F(TextureTest, BoxTexturedGlbTextureSampling) {
    // BoxTextured.glb をロードしてテクスチャデータを取得
    GltfData gltf;
    ASSERT_TRUE(gltf.load("assets/BoxTextured.glb"));

    auto image = gltf.getTextureImage(0);
    ASSERT_GT(image.width, 0);
    ASSERT_GT(image.height, 0);
    ASSERT_FALSE(image.pixels.empty());

    // テクスチャデータをLuaに渡してサンプリング
    lua["test_width"] = image.width;
    lua["test_height"] = image.height;
    lua["test_channels"] = image.channels;

    // ピクセルデータをLuaテーブルに変換
    sol::table pixels = lua.create_table(static_cast<int>(image.pixels.size()), 0);
    for (size_t i = 0; i < image.pixels.size(); ++i) {
        pixels[i + 1] = static_cast<int>(image.pixels[i]);
    }
    lua["test_pixels"] = pixels;

    auto result = lua.safe_script(R"(
        local Texture = require('lib.Texture')
        local tex = Texture.new(test_width, test_height, test_channels, test_pixels)
        assert(tex ~= nil, "テクスチャ生成に失敗")
        assert(tex.width == test_width, "幅が一致しない")

        -- 中央付近をサンプリングして値が有効か確認
        local r, g, b = tex:sample(0.5, 0.5)
        assert(r >= 0 and r <= 255, "rが範囲外: " .. r)
        assert(g >= 0 and g <= 255, "gが範囲外: " .. g)
        assert(b >= 0 and b <= 255, "bが範囲外: " .. b)

        return true
    )", sol::script_pass_on_error);
    ASSERT_TRUE(result.valid()) << "Lua error: " << sol::error(result).what();
    EXPECT_TRUE(result.get<bool>());
}
