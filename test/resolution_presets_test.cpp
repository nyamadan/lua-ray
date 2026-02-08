// テスト: ResolutionPresetsモジュール
// TDDアプローチ: まずテストを書いて失敗させる (RED Phase)

#include <gtest/gtest.h>
#include "../src/lua_binding.h"
#include <sol/sol.hpp>

class ResolutionPresetsTest : public ::testing::Test {
protected:
    void SetUp() override {
        AppContext ctx;
        bind_lua(lua, ctx);
        // lib ディレクトリへのパスを追加
        lua.script("package.path = package.path .. ';./lib/?.lua;../../?.lua'");
    }

    sol::state lua;
};

// テスト1: プリセットリストを取得できる
TEST_F(ResolutionPresetsTest, GetPresets) {
    auto result = lua.safe_script(R"(
        local ResolutionPresets = require('lib.ResolutionPresets')
        return ResolutionPresets.get_presets()
    )");
    
    ASSERT_TRUE(result.valid()) << ((sol::error)result).what();
    
    sol::table presets = result;
    ASSERT_GE(presets.size(), 3); // 最低3つのプリセットがある
    
    // 最初のプリセットにname, width, heightがある
    sol::table first = presets[1];
    ASSERT_TRUE(first["name"].valid());
    ASSERT_TRUE(first["width"].valid());
    ASSERT_TRUE(first["height"].valid());
}

// テスト2: デフォルトプリセットインデックスを取得できる
TEST_F(ResolutionPresetsTest, GetDefaultIndex) {
    auto result = lua.safe_script(R"(
        local ResolutionPresets = require('lib.ResolutionPresets')
        return ResolutionPresets.get_default_index()
    )");
    
    ASSERT_TRUE(result.valid()) << ((sol::error)result).what();
    int index = result;
    ASSERT_GE(index, 1); // Luaは1-indexed
}

// テスト3: インデックスから解像度を取得できる
TEST_F(ResolutionPresetsTest, GetResolutionByIndex) {
    auto result = lua.safe_script(R"(
        local ResolutionPresets = require('lib.ResolutionPresets')
        local w, h = ResolutionPresets.get_resolution(1)
        return w, h
    )");
    
    ASSERT_TRUE(result.valid()) << ((sol::error)result).what();
    std::tuple<int, int> res = result;
    ASSERT_GT(std::get<0>(res), 0); // width > 0
    ASSERT_GT(std::get<1>(res), 0); // height > 0
}

// テスト4: フィット矩形を計算できる（アスペクト比維持）
// テクスチャサイズ、ウィンドウサイズから中央揃えフィット矩形を計算
TEST_F(ResolutionPresetsTest, CalculateFitRect) {
    auto result = lua.safe_script(R"(
        local ResolutionPresets = require('lib.ResolutionPresets')
        
        -- テクスチャ: 800x600, ウィンドウ: 1024x768
        local x, y, w, h = ResolutionPresets.calculate_fit_rect(800, 600, 1024, 768)
        return x, y, w, h
    )");
    
    ASSERT_TRUE(result.valid()) << ((sol::error)result).what();
    std::tuple<float, float, float, float> rect = result;
    
    float x = std::get<0>(rect);
    float y = std::get<1>(rect);
    float w = std::get<2>(rect);
    float h = std::get<3>(rect);
    
    // アスペクト比を維持するため、幅または高さがウィンドウにフィット
    // 800x600 (4:3) を 1024x768 (4:3) に → 1024x768 にスケール
    ASSERT_FLOAT_EQ(w, 1024.0f);
    ASSERT_FLOAT_EQ(h, 768.0f);
    ASSERT_FLOAT_EQ(x, 0.0f); // 中央揃え
    ASSERT_FLOAT_EQ(y, 0.0f);
}

// テスト5: 横長テクスチャのフィット計算
TEST_F(ResolutionPresetsTest, CalculateFitRectWideTexture) {
    auto result = lua.safe_script(R"(
        local ResolutionPresets = require('lib.ResolutionPresets')
        
        -- テクスチャ: 1920x1080 (16:9), ウィンドウ: 800x600 (4:3)
        local x, y, w, h = ResolutionPresets.calculate_fit_rect(1920, 1080, 800, 600)
        return x, y, w, h
    )");
    
    ASSERT_TRUE(result.valid()) << ((sol::error)result).what();
    std::tuple<float, float, float, float> rect = result;
    
    float x = std::get<0>(rect);
    float y = std::get<1>(rect);
    float w = std::get<2>(rect);
    float h = std::get<3>(rect);
    
    // 16:9のテクスチャを4:3ウィンドウに収める → 幅がウィンドウにフィット
    ASSERT_FLOAT_EQ(w, 800.0f);
    ASSERT_FLOAT_EQ(h, 450.0f); // 800 * (1080/1920) = 450
    ASSERT_FLOAT_EQ(x, 0.0f);
    ASSERT_FLOAT_EQ(y, 75.0f); // (600 - 450) / 2 = 75
}
