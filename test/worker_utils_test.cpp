// worker_utils_test.cpp
// TDD Red Phase: WorkerUtils Luaモジュールのテスト

#include <gtest/gtest.h>
#include <sol/sol.hpp>
#include "../src/app_data.h"

class WorkerUtilsTest : public ::testing::Test {
protected:
    void SetUp() override {
        lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::math, sol::lib::table, sol::lib::os, sol::lib::string, sol::lib::coroutine);
        
        // プロジェクトルートから実行されることを前提にパス設定
        lua.script("package.path = './?.lua;./workers/?.lua;' .. package.path");
    }

    sol::state lua;
};

// block_process_loop のテスト (モック使用)
TEST_F(WorkerUtilsTest, BlockProcessLoopRunsCallback) {
    // AppDataモック
    lua.new_usertype<AppData>("MockAppData",
        "set_string", &AppData::set_string,
        "get_string", &AppData::get_string,
        "pop_next_index", &AppData::pop_next_index
    );
    
    AppData data(100, 100);
    lua["app_data"] = &data;
    
    // BlockUtilsとjsonのモック/読み込み
    // 実際のBlockUtilsを使う
    
    // app.get_ticks のモックをC++側で定義 (確実にするため)
    sol::table app = lua.create_table();
    app.set_function("get_ticks", []() { return 0; });
    lua["app"] = app;

    lua.script(R"(
        local WorkerUtils = require("workers.worker_utils")
        local BlockUtils = require("lib.BlockUtils")
        
        -- テスト用のブロックをセットアップ
        local blocks = {
            {x = 0, y = 0, w = 10, h = 10}
        }
        BlockUtils.setup_shared_queue(app_data, blocks, "test_queue", "test_queue_idx")
        
        -- コールバック呼び出し回数
        call_count = 0
        
        local function process_callback(app_data, x, y)
            call_count = call_count + 1
        end
        
        -- キャンセルチェック関数 (常にfalse=キャンセルしない)
        local function check_cancel()
            return false
        end
        
        -- appはC++で定義済みなのでそのまま渡す、もしくはnilで渡してグローバルフォールバックを確認してもよい
        WorkerUtils.process_blocks(app_data, "test_queue", "test_queue_idx", process_callback, check_cancel, app)
    )");
    
    int call_count = lua["call_count"];
    // 10x10 = 100回呼ばれるはず
    ASSERT_EQ(call_count, 100);
}

// キャンセル時の動作テスト
TEST_F(WorkerUtilsTest, BlockProcessLoopStopsOnCancel) {
     // AppDataモック
    lua.new_usertype<AppData>("MockAppData",
        "set_string", &AppData::set_string,
        "get_string", &AppData::get_string,
        "pop_next_index", &AppData::pop_next_index
    );
    
    AppData data(100, 100);
    lua["app_data"] = &data;
    
    // app.get_ticks のモックをC++側で定義
    sol::table app = lua.create_table();
    // call_countにアクセスしたいが、C++ラムダからは直接Lua変数は見えない。
    // 代わりにLua側の変数を参照する関数にするか、Lua側で関数を上書きするか。
    // ここではシンプルにLua側で定義した変数を参照するようにしたいが、sol2だと少し手間。
    // なので、appテーブルだけ作って、get_ticksはLua側で定義/上書きする方針にする。
    lua["app"] = app;
    
    lua.script(R"(
        local WorkerUtils = require("workers.worker_utils")
        local BlockUtils = require("lib.BlockUtils")
        
        -- テスト用のブロックをセットアップ
        local blocks = {
            {x = 0, y = 0, w = 100, h = 100} -- 大きめのブロック
        }
        BlockUtils.setup_shared_queue(app_data, blocks, "test_queue_cancel", "test_queue_idx_cancel")
        
        call_count = 0
        
        local function process_callback(app_data, x, y)
            call_count = call_count + 1
        end
        
        -- 10回呼ばれたらキャンセルする
        local function check_cancel()
            return call_count >= 10
        end
        
        -- app.get_ticks を定義 (call_count依存)
        app.get_ticks = function() return call_count * 10 end
        
        WorkerUtils.process_blocks(app_data, "test_queue_cancel", "test_queue_idx_cancel", process_callback, check_cancel, app)
    )");
    
    int call_count = lua["call_count"];
    ASSERT_LT(call_count, 10000); // 全部終わる前に止まるはず
    ASSERT_GE(call_count, 10);    // 少なくとも10回は動く
}

// テスト: on_block_complete コールバックが各ブロック完了後に呼ばれる
TEST_F(WorkerUtilsTest, OnBlockCompleteCallbackCalledPerBlock) {
    lua.new_usertype<AppData>("MockAppData",
        "set_string", &AppData::set_string,
        "get_string", &AppData::get_string,
        "pop_next_index", &AppData::pop_next_index
    );
    
    AppData data(100, 100);
    lua["app_data"] = &data;
    
    sol::table app = lua.create_table();
    app.set_function("get_ticks", []() { return 0; });
    lua["app"] = app;

    lua.script(R"(
        local WorkerUtils = require("workers.worker_utils")
        local BlockUtils = require("lib.BlockUtils")
        
        -- 3つのブロックをセットアップ
        local blocks = {
            {x = 0, y = 0, w = 5, h = 5},
            {x = 5, y = 0, w = 5, h = 5},
            {x = 0, y = 5, w = 5, h = 5}
        }
        BlockUtils.setup_shared_queue(app_data, blocks, "test_obc_queue", "test_obc_idx")
        
        pixel_count = 0
        block_complete_count = 0
        
        local function process_callback(app_data, x, y)
            pixel_count = pixel_count + 1
        end
        
        local function check_cancel()
            return false
        end
        
        -- ブロック完了コールバック
        local function on_block_complete()
            block_complete_count = block_complete_count + 1
        end
        
        WorkerUtils.process_blocks(app_data, "test_obc_queue", "test_obc_idx", process_callback, check_cancel, app, on_block_complete)
    )");
    
    int pixel_count = lua["pixel_count"];
    int block_complete_count = lua["block_complete_count"];
    // 3ブロック × 5×5 = 75ピクセル
    ASSERT_EQ(pixel_count, 75);
    // 3ブロック完了コールバック
    ASSERT_EQ(block_complete_count, 3);
}
