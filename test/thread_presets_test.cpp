// テスト: ThreadPresetsモジュール
// TDDアプローチ: まずテストを書いて失敗させる (RED Phase)

#include <gtest/gtest.h>
#include "../src/lua_binding.h"
#include <sol/sol.hpp>

class ThreadPresetsTest : public ::testing::Test {
protected:
    void SetUp() override {
        AppContext ctx;
        bind_lua(lua, ctx);
        // lib ディレクトリへのパスを追加
        lua.script("package.path = package.path .. ';./lib/?.lua;../../?.lua'");
    }

    sol::state lua;
};

// テスト1: スレッド数プリセットリストを取得できる
TEST_F(ThreadPresetsTest, GetThreadPresets) {
    auto result = lua.safe_script(R"(
        local ThreadPresets = require('lib.ThreadPresets')
        return ThreadPresets.get_thread_presets()
    )");
    
    ASSERT_TRUE(result.valid()) << ((sol::error)result).what();
    
    sol::table presets = result;
    ASSERT_GE(presets.size(), 3); // 最低3つのプリセットがある
    
    // 各プリセットにvalueとnameがある
    sol::table first = presets[1];
    ASSERT_TRUE(first["value"].valid());
    ASSERT_TRUE(first["name"].valid());
}

// テスト2: ブロックサイズプリセットリストを取得できる
TEST_F(ThreadPresetsTest, GetBlockPresets) {
    auto result = lua.safe_script(R"(
        local ThreadPresets = require('lib.ThreadPresets')
        return ThreadPresets.get_block_presets()
    )");
    
    ASSERT_TRUE(result.valid()) << ((sol::error)result).what();
    
    sol::table presets = result;
    ASSERT_GE(presets.size(), 3); // 最低3つのプリセットがある
    
    sol::table first = presets[1];
    ASSERT_TRUE(first["value"].valid());
    ASSERT_TRUE(first["name"].valid());
}

// テスト3: スレッド数デフォルトインデックスを取得できる
TEST_F(ThreadPresetsTest, GetDefaultThreadIndex) {
    auto result = lua.safe_script(R"(
        local ThreadPresets = require('lib.ThreadPresets')
        local idx = ThreadPresets.get_default_thread_index()
        local presets = ThreadPresets.get_thread_presets()
        return idx, presets[idx].value
    )");
    
    ASSERT_TRUE(result.valid()) << ((sol::error)result).what();
    std::tuple<int, int> res = result;
    ASSERT_GE(std::get<0>(res), 1); // Luaは1-indexed
    ASSERT_EQ(std::get<1>(res), 8); // デフォルトは8スレッド
}

// テスト4: ブロックサイズデフォルトインデックスを取得できる
TEST_F(ThreadPresetsTest, GetDefaultBlockIndex) {
    auto result = lua.safe_script(R"(
        local ThreadPresets = require('lib.ThreadPresets')
        local idx = ThreadPresets.get_default_block_index()
        local presets = ThreadPresets.get_block_presets()
        return idx, presets[idx].value
    )");
    
    ASSERT_TRUE(result.valid()) << ((sol::error)result).what();
    std::tuple<int, int> res = result;
    ASSERT_GE(std::get<0>(res), 1);
    ASSERT_EQ(std::get<1>(res), 64); // デフォルトは64
}

// テスト5: 値からスレッド数インデックスを逆引きできる
TEST_F(ThreadPresetsTest, FindThreadIndexByValue) {
    auto result = lua.safe_script(R"(
        local ThreadPresets = require('lib.ThreadPresets')
        local idx_8 = ThreadPresets.find_thread_index(8)
        local idx_4 = ThreadPresets.find_thread_index(4)
        local idx_nil = ThreadPresets.find_thread_index(999)
        return idx_8, idx_4, idx_nil
    )");
    
    ASSERT_TRUE(result.valid()) << ((sol::error)result).what();
    std::tuple<int, int, sol::object> res = result;
    ASSERT_GT(std::get<0>(res), 0); // 8スレッドのインデックスが見つかる
    ASSERT_GT(std::get<1>(res), 0); // 4スレッドのインデックスが見つかる
    ASSERT_TRUE(std::get<2>(res).is<sol::nil_t>()); // 999は見つからない
}

// テスト6: 値からブロックサイズインデックスを逆引きできる
TEST_F(ThreadPresetsTest, FindBlockIndexByValue) {
    auto result = lua.safe_script(R"(
        local ThreadPresets = require('lib.ThreadPresets')
        local idx_64 = ThreadPresets.find_block_index(64)
        local idx_128 = ThreadPresets.find_block_index(128)
        local idx_nil = ThreadPresets.find_block_index(999)
        return idx_64, idx_128, idx_nil
    )");
    
    ASSERT_TRUE(result.valid()) << ((sol::error)result).what();
    std::tuple<int, int, sol::object> res = result;
    ASSERT_GT(std::get<0>(res), 0); // 64のインデックスが見つかる
    ASSERT_GT(std::get<1>(res), 0); // 128のインデックスが見つかる
    ASSERT_TRUE(std::get<2>(res).is<sol::nil_t>()); // 999は見つからない
}
