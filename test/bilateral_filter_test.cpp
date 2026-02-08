// test/bilateral_filter_test.cpp
// BilateralFilterモジュールのユニットテスト（t-wada TDD: Red Phase）

#include <gtest/gtest.h>
#include <sol/sol.hpp>
#include <tuple>
#include <cmath>
#include "app_data.h"

class BilateralFilterTest : public ::testing::Test {
protected:
    void SetUp() override {
        lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::math, sol::lib::table);
        // lib ディレクトリを package.path に追加
        lua.script("package.path = package.path .. ';./lib/?.lua;../../?.lua'");
        
        // AppData バインディング
        lua.new_usertype<AppData>("AppData",
            sol::constructors<AppData(int, int)>(),
            "set_pixel", &AppData::set_pixel,
            "get_pixel", &AppData::get_pixel,
            "swap", &AppData::swap,
            "width", &AppData::get_width,
            "height", &AppData::get_height,
            "clear", &AppData::clear
        );
    }

    sol::state lua;
};

// ===========================================
// 基本テスト: モジュール読み込み
// ===========================================

TEST_F(BilateralFilterTest, CanRequireBilateralFilter) {
    auto result = lua.safe_script("return require('lib.BilateralFilter')");
    ASSERT_TRUE(result.valid()) << "BilateralFilterモジュールが読み込めません";
}

// ===========================================
// ガウス関数テスト
// ===========================================

TEST_F(BilateralFilterTest, GaussianAtZero) {
    // x=0 でガウス関数は常に1を返す
    auto result = lua.safe_script(R"(
        local BF = require('lib.BilateralFilter')
        return BF.gaussian(0, 1.0)
    )");
    ASSERT_TRUE(result.valid());
    double value = result.get<double>();
    ASSERT_NEAR(value, 1.0, 1e-5);
}

TEST_F(BilateralFilterTest, GaussianDecreases) {
    // x が大きくなるとガウス関数の値は小さくなる
    auto result = lua.safe_script(R"(
        local BF = require('lib.BilateralFilter')
        local g0 = BF.gaussian(0, 1.0)
        local g1 = BF.gaussian(1, 1.0)
        local g2 = BF.gaussian(2, 1.0)
        return g0 > g1 and g1 > g2 and g2 > 0
    )");
    ASSERT_TRUE(result.valid());
    bool decreases = result.get<bool>();
    ASSERT_TRUE(decreases);
}

TEST_F(BilateralFilterTest, GaussianWithSigma) {
    // σ=2 で x=2 の時: exp(-4/(2*4)) = exp(-0.5)
    auto result = lua.safe_script(R"(
        local BF = require('lib.BilateralFilter')
        return BF.gaussian(2, 2.0)
    )");
    ASSERT_TRUE(result.valid());
    double value = result.get<double>();
    double expected = std::exp(-0.5);
    ASSERT_NEAR(value, expected, 1e-5);
}

// ===========================================
// フィルタテスト
// ===========================================

TEST_F(BilateralFilterTest, FilterUniformColor) {
    // 均一な色の画像にフィルタを適用すると同じ色になる
    auto result = lua.safe_script(R"(
        local BF = require('lib.BilateralFilter')
        local data = AppData.new(10, 10)
        
        -- 全ピクセルを赤(128, 0, 0)に設定
        for y = 0, 9 do
            for x = 0, 9 do
                data:set_pixel(x, y, 128, 0, 0)
            end
        end
        data:swap()
        
        -- 中心ピクセルにフィルタ適用
        local r, g, b = BF.filter(data, 5, 5)
        return r, g, b
    )");
    ASSERT_TRUE(result.valid());
    auto [r, g, b] = result.get<std::tuple<int, int, int>>();
    ASSERT_EQ(r, 128);
    ASSERT_EQ(g, 0);
    ASSERT_EQ(b, 0);
}

TEST_F(BilateralFilterTest, FilterSinglePixel) {
    // 1x1画像でフィルタを適用すると同じ色になる
    auto result = lua.safe_script(R"(
        local BF = require('lib.BilateralFilter')
        local data = AppData.new(1, 1)
        data:set_pixel(0, 0, 255, 128, 64)
        data:swap()
        
        local r, g, b = BF.filter(data, 0, 0)
        return r, g, b
    )");
    ASSERT_TRUE(result.valid());
    auto [r, g, b] = result.get<std::tuple<int, int, int>>();
    ASSERT_EQ(r, 255);
    ASSERT_EQ(g, 128);
    ASSERT_EQ(b, 64);
}

TEST_F(BilateralFilterTest, FilterPreservesEdge) {
    // エッジ保持: 大きな色差があるピクセルは影響を受けにくい
    auto result = lua.safe_script(R"(
        local BF = require('lib.BilateralFilter')
        local data = AppData.new(5, 5)
        
        -- 左半分を白、右半分を黒に設定
        for y = 0, 4 do
            for x = 0, 4 do
                if x < 2 then
                    data:set_pixel(x, y, 255, 255, 255)
                else
                    data:set_pixel(x, y, 0, 0, 0)
                end
            end
        end
        data:swap()
        
        -- 境界付近のピクセル(x=2)にフィルタ適用
        -- 黒いピクセルなのでフィルタ後も黒に近いはず
        local r, g, b = BF.filter(data, 2, 2, {radius = 1, sigma_color = 0.1})
        
        -- 黒に近い値であることを確認（エッジが保持されている）
        return r < 50 and g < 50 and b < 50
    )");
    ASSERT_TRUE(result.valid());
    bool edgePreserved = result.get<bool>();
    ASSERT_TRUE(edgePreserved) << "エッジが保持されていません";
}

TEST_F(BilateralFilterTest, FilterWithCustomParams) {
    // カスタムパラメータでフィルタが動作する
    auto result = lua.safe_script(R"(
        local BF = require('lib.BilateralFilter')
        local data = AppData.new(5, 5)
        
        for y = 0, 4 do
            for x = 0, 4 do
                data:set_pixel(x, y, 100, 100, 100)
            end
        end
        data:swap()
        
        local r, g, b = BF.filter(data, 2, 2, {
            radius = 2,
            sigma_spatial = 1.0,
            sigma_color = 0.5
        })
        return r, g, b
    )");
    ASSERT_TRUE(result.valid());
    auto [r, g, b] = result.get<std::tuple<int, int, int>>();
    // 浮動小数点丸め誤差で±1の差が生じることがある
    ASSERT_NEAR(r, 100, 1);
    ASSERT_NEAR(g, 100, 1);
    ASSERT_NEAR(b, 100, 1);
}
