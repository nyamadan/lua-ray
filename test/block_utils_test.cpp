// block_utils_test.cpp
// TDD Red Phase: BlockUtils Luaモジュールのテスト

#include <gtest/gtest.h>
#include <sol/sol.hpp>
#include "../src/app_data.h"

class BlockUtilsTest : public ::testing::Test {
protected:
    void SetUp() override {
        lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::math, sol::lib::table, sol::lib::os, sol::lib::string);
        
        // プロジェクトルートから実行されることを前提に、シンプルにパスを追加
        // BlockUtils.lua は require("lib.json") を行う
        // ./?.lua により lib/json.lua が見つかるはず
        lua.script("package.path = './?.lua;' .. package.path");
    }

    sol::state lua;
};

// ========================================
// MovingAverage テスト
// ========================================

// テスト1: MovingAverageの初期値は0
TEST_F(BlockUtilsTest, MovingAverageInitialValueIsZero) {
    lua.script(R"(
        local BlockUtils = require("lib.BlockUtils")
        local avg = BlockUtils.MovingAverage.new(0.1)
        result = avg:get()
    )");
    
    double result = lua["result"];
    ASSERT_DOUBLE_EQ(result, 0.0);
}

// テスト2: 最初のupdateで値がセットされる
TEST_F(BlockUtilsTest, MovingAverageFirstUpdate) {
    lua.script(R"(
        local BlockUtils = require("lib.BlockUtils")
        local avg = BlockUtils.MovingAverage.new(0.1)
        avg:update(10.0)
        result = avg:get()
    )");
    
    double result = lua["result"];
    // 初回は値がそのままセットされる
    ASSERT_DOUBLE_EQ(result, 10.0);
}

// テスト3: 指数移動平均が正しく計算される
TEST_F(BlockUtilsTest, MovingAverageExponentialCalculation) {
    lua.script(R"(
        local BlockUtils = require("lib.BlockUtils")
        local avg = BlockUtils.MovingAverage.new(0.5) -- alpha = 0.5
        avg:update(10.0)  -- 初回: 10.0
        avg:update(20.0)  -- EMA = 0.5 * 20 + 0.5 * 10 = 15.0
        result = avg:get()
    )");
    
    double result = lua["result"];
    ASSERT_DOUBLE_EQ(result, 15.0);
}

// テスト4: 複数回の更新で平滑化される
TEST_F(BlockUtilsTest, MovingAverageMultipleUpdates) {
    lua.script(R"(
        local BlockUtils = require("lib.BlockUtils")
        local avg = BlockUtils.MovingAverage.new(0.2)
        avg:update(100.0)  -- 初回: 100.0
        avg:update(100.0)  -- EMA = 0.2 * 100 + 0.8 * 100 = 100.0
        avg:update(0.0)    -- EMA = 0.2 * 0 + 0.8 * 100 = 80.0
        avg:update(0.0)    -- EMA = 0.2 * 0 + 0.8 * 80 = 64.0
        result = avg:get()
    )");
    
    double result = lua["result"];
    ASSERT_DOUBLE_EQ(result, 64.0);
}

// ========================================
// generate_blocks テスト
// ========================================

// テスト5: 小さい画面で1ブロックのみ生成
TEST_F(BlockUtilsTest, GenerateBlocksSingleBlock) {
    lua.script(R"(
        local BlockUtils = require("lib.BlockUtils")
        local blocks = BlockUtils.generate_blocks(32, 32, 64, 4)
        result_count = #blocks
        result_first = blocks[1]
    )");
    
    int count = lua["result_count"];
    ASSERT_EQ(count, 1);
    
    sol::table first = lua["result_first"];
    ASSERT_EQ(first["x"].get<int>(), 0);
    ASSERT_EQ(first["y"].get<int>(), 0);
    ASSERT_EQ(first["w"].get<int>(), 32);
    ASSERT_EQ(first["h"].get<int>(), 32);
}

// テスト6: 画面全体がブロックでカバーされる
TEST_F(BlockUtilsTest, GenerateBlocksCoversEntireScreen) {
    lua.script(R"(
        local BlockUtils = require("lib.BlockUtils")
        local blocks = BlockUtils.generate_blocks(128, 128, 64, 4)
        
        -- ピクセルカバレッジを集計
        local covered = {}
        for _, block in ipairs(blocks) do
            for y = block.y, block.y + block.h - 1 do
                for x = block.x, block.x + block.w - 1 do
                    local key = y * 128 + x
                    covered[key] = true
                end
            end
        end
        
        -- 128x128 = 16384ピクセル全てがカバーされているか
        covered_count = 0
        for _ in pairs(covered) do
            covered_count = covered_count + 1
        end
    )");
    
    int covered_count = lua["covered_count"];
    ASSERT_EQ(covered_count, 128 * 128);
}

// テスト7: ブロックがスレッドに分配される
TEST_F(BlockUtilsTest, GenerateBlocksDistributesToThreads) {
    lua.script(R"(
        local BlockUtils = require("lib.BlockUtils")
        local blocks = BlockUtils.generate_blocks(256, 256, 64, 4)
        
        -- 各スレッドIDをカウント
        thread_counts = {}
        for _, block in ipairs(blocks) do
            local tid = block.thread_id
            thread_counts[tid] = (thread_counts[tid] or 0) + 1
        end
        
        result_count = #blocks
    )");
    
    int count = lua["result_count"];
    // 256x256を64x64ブロックで分割 = 4x4 = 16ブロック
    ASSERT_EQ(count, 16);
    
    sol::table thread_counts = lua["thread_counts"];
    // 4スレッドにほぼ均等に分配されているはず
    for (int i = 0; i < 4; i++) {
        int thread_block_count = thread_counts[i].get_or(0);
        ASSERT_GE(thread_block_count, 3);  // 最低3ブロック
        ASSERT_LE(thread_block_count, 5);  // 最大5ブロック
    }
}

// テスト8: 端数が正しく処理される
TEST_F(BlockUtilsTest, GenerateBlocksHandlesRemainder) {
    lua.script(R"(
        local BlockUtils = require("lib.BlockUtils")
        -- 100x100を64x64で分割: 2x2ブロック、端が36ピクセル
        local blocks = BlockUtils.generate_blocks(100, 100, 64, 2)
        
        result_count = #blocks
        
        -- ピクセルカバレッジを集計
        local covered = {}
        for _, block in ipairs(blocks) do
            for y = block.y, block.y + block.h - 1 do
                for x = block.x, block.x + block.w - 1 do
                    local key = y * 100 + x
                    covered[key] = true
                end
            end
        end
        
        covered_count = 0
        for _ in pairs(covered) do
            covered_count = covered_count + 1
        end
    )");
    
    int covered_count = lua["covered_count"];
    ASSERT_EQ(covered_count, 100 * 100);
}

// ========================================
// shuffle_blocks テスト
// ========================================

// テスト9: シャッフル後も全ブロックが保持される
TEST_F(BlockUtilsTest, ShuffleBlocksReturnsAllBlocks) {
    lua.script(R"(
        local BlockUtils = require("lib.BlockUtils")
        local blocks = BlockUtils.generate_blocks(256, 256, 64, 4)
        local original_count = #blocks
        
        local shuffled = BlockUtils.shuffle_blocks(blocks, 12345)
        result_count = #shuffled
        original_count_result = original_count
    )");
    
    int original_count = lua["original_count_result"];
    int shuffled_count = lua["result_count"];
    ASSERT_EQ(shuffled_count, original_count);
}

// テスト10: 同じシードで同じ結果になる（決定論的）
TEST_F(BlockUtilsTest, ShuffleBlocksWithSeedIsDeterministic) {
    lua.script(R"(
        local BlockUtils = require("lib.BlockUtils")
        local blocks = BlockUtils.generate_blocks(128, 128, 64, 2)
        
        local shuffled1 = BlockUtils.shuffle_blocks(blocks, 42)
        local shuffled2 = BlockUtils.shuffle_blocks(blocks, 42)
        
        -- 両方の順序が同じか確認
        result_same = true
        for i = 1, #shuffled1 do
            if shuffled1[i].x ~= shuffled2[i].x or shuffled1[i].y ~= shuffled2[i].y then
                result_same = false
                break
            end
        end
    )");
    
    bool result_same = lua["result_same"];
    ASSERT_TRUE(result_same);
}

// テスト11: シャッフルにより順序が変わる（異なるシードで異なる結果）
TEST_F(BlockUtilsTest, ShuffleBlocksChangesOrder) {
    lua.script(R"(
        local BlockUtils = require("lib.BlockUtils")
        local blocks = BlockUtils.generate_blocks(256, 256, 64, 4)
        
        local shuffled1 = BlockUtils.shuffle_blocks(blocks, 123)
        local shuffled2 = BlockUtils.shuffle_blocks(blocks, 456)
        
        -- 異なるシードで異なる順序になるか確認
        result_different = false
        for i = 1, #shuffled1 do
            if shuffled1[i].x ~= shuffled2[i].x or shuffled1[i].y ~= shuffled2[i].y then
                result_different = true
                break
            end
        end
    )");
    
    bool result_different = lua["result_different"];
    ASSERT_TRUE(result_different);
}

// テスト12: 元の配列は変更されない
TEST_F(BlockUtilsTest, ShuffleBlocksDoesNotModifyOriginal) {
    lua.script(R"(
        local BlockUtils = require("lib.BlockUtils")
        local blocks = BlockUtils.generate_blocks(128, 128, 64, 2)
        
        -- 元の最初のブロックを記録
        original_first_x = blocks[1].x
        original_first_y = blocks[1].y
        
        -- シャッフル実行
        local shuffled = BlockUtils.shuffle_blocks(blocks, 99999)
        
        -- 元の配列が変更されていないか確認
        after_first_x = blocks[1].x
        after_first_y = blocks[1].y
    )");
    
    int original_x = lua["original_first_x"];
    int original_y = lua["original_first_y"];
    int after_x = lua["after_first_x"];
    int after_y = lua["after_first_y"];
    
    ASSERT_EQ(original_x, after_x);
    ASSERT_EQ(original_y, after_y);
}

// ========================================
// sort_blocks_by_distance テスト
// ========================================

// テスト13: 中心点から近い順にソートされる
TEST_F(BlockUtilsTest, SortBlocksByDistanceBasic) {
    lua.script(R"(
        local BlockUtils = require("lib.BlockUtils")
        
        -- テスト用のブロック定義 (10x10のブロック)
        local blocks = {
            {x = 0, y = 0, w = 10, h = 10},     -- 中心 (5, 5)   距離 (45, 45) -> 4050
            {x = 40, y = 40, w = 10, h = 10},   -- 中心 (45, 45) 距離 (5, 5) -> 50
            {x = 50, y = 50, w = 10, h = 10},   -- 中心 (55, 55) 距離 (5, 5) -> 50
            {x = 100, y = 100, w = 10, h = 10}  -- 中心 (105, 105) 距離 (55, 55) -> 6050
        }
        
        -- 目標中心点: (50, 50)
        local sorted = BlockUtils.sort_blocks_by_distance(blocks, 50, 50)
        
        -- 近い順に並んでいるか確認
        first_x = sorted[1].x
        first_y = sorted[1].y
        second_x = sorted[2].x
        second_y = sorted[2].y
        fourth_x = sorted[4].x
        fourth_y = sorted[4].y
        
        count = #sorted
    )");
    
    int count = lua["count"];
    ASSERT_EQ(count, 4);
    
    int first_x = lua["first_x"];
    int first_y = lua["first_y"];
    // (40, 40) または (50, 50) が先頭か2番目に来るはず
    // 中心点との距離はどちらも同じなので、安定ソートかどうかに依存するが
    // 少なくとも (0,0) や (100,100) が先頭には来ない
    ASSERT_TRUE((first_x == 40 && first_y == 40) || (first_x == 50 && first_y == 50));
    
    int fourth_x = lua["fourth_x"];
    int fourth_y = lua["fourth_y"];
    // 最も遠い (100, 100) が最後に来る
    ASSERT_EQ(fourth_x, 100);
    ASSERT_EQ(fourth_y, 100);
}

// テスト14: ソート後も全ブロックが保持される
TEST_F(BlockUtilsTest, SortBlocksByDistancePreservesAllBlocks) {
    lua.script(R"(
        local BlockUtils = require("lib.BlockUtils")
        local blocks = BlockUtils.generate_blocks(256, 256, 64, 4)
        local original_count = #blocks
        
        local sorted = BlockUtils.sort_blocks_by_distance(blocks, 128, 128)
        result_count = #sorted
        original_count_result = original_count
    )");
    
    int original_count = lua["original_count_result"];
    int sorted_count = lua["result_count"];
    ASSERT_EQ(sorted_count, original_count);
}

// テスト15: 元の配列は変更されない
TEST_F(BlockUtilsTest, SortBlocksDoesNotModifyOriginal) {
    lua.script(R"(
        local BlockUtils = require("lib.BlockUtils")
        local blocks = BlockUtils.generate_blocks(128, 128, 64, 2)
        
        -- 元の最初のブロックを記録
        original_first_x = blocks[1].x
        original_first_y = blocks[1].y
        
        -- ソート実行
        local sorted = BlockUtils.sort_blocks_by_distance(blocks, 100, 100)
        
        -- 元の配列が変更されていないか確認
        after_first_x = blocks[1].x
        after_first_y = blocks[1].y
    )");
    
    int original_x = lua["original_first_x"];
    int original_y = lua["original_first_y"];
    int after_x = lua["after_first_x"];
    int after_y = lua["after_first_y"];
    
    ASSERT_EQ(original_x, after_x);
    ASSERT_EQ(original_y, after_y);
}

// ========================================
// 共有キュー関連 テスト（TDD Red Phase）
// ========================================

// 注: これらのテストはAppDataモックを使用するため、
// Lua単体ではなくapp_dataを注入してテストする必要がある

// テスト13: setup_shared_queue がブロック配列をJSONで保存
TEST_F(BlockUtilsTest, SetupSharedQueueStoresBlocksAsJson) {
    // AppDataモックのバインディング
    lua.new_usertype<AppData>("MockAppData",
        "set_string", &AppData::set_string,
        "get_string", &AppData::get_string,
        "pop_next_index", &AppData::pop_next_index
    );
    
    AppData data(100, 100);
    lua["app_data"] = &data;
    
    lua.script(R"(
        local BlockUtils = require("lib.BlockUtils")
        local json = require("lib.json")
        
        local blocks = {
            {x = 0, y = 0, w = 64, h = 64},
            {x = 64, y = 0, w = 36, h = 64}
        }
        
        BlockUtils.setup_shared_queue(app_data, blocks, "test_queue")
        
        -- 保存されたデータを確認
        stored_json = app_data:get_string("test_queue")
        stored_blocks = json.decode(stored_json)
    )");
    
    sol::table stored_blocks = lua["stored_blocks"];
    ASSERT_EQ(stored_blocks.size(), 2);
    
    sol::table first = stored_blocks[1];
    ASSERT_EQ(first["x"].get<int>(), 0);
    ASSERT_EQ(first["y"].get<int>(), 0);
}

// テスト14: pull_next_block が順番にブロックを返す
TEST_F(BlockUtilsTest, PullNextBlockReturnsBlocksInOrder) {
    lua.new_usertype<AppData>("MockAppData",
        "set_string", &AppData::set_string,
        "get_string", &AppData::get_string,
        "pop_next_index", &AppData::pop_next_index
    );
    
    AppData data(100, 100);
    lua["app_data"] = &data;
    
    lua.script(R"(
        local BlockUtils = require("lib.BlockUtils")
        
        local blocks = {
            {x = 0, y = 0, w = 64, h = 64},
            {x = 64, y = 0, w = 36, h = 64},
            {x = 0, y = 64, w = 64, h = 36}
        }
        
        BlockUtils.setup_shared_queue(app_data, blocks, "queue", "queue_idx")
        
        -- 順番にpull
        block1 = BlockUtils.pull_next_block(app_data, "queue", "queue_idx")
        block2 = BlockUtils.pull_next_block(app_data, "queue", "queue_idx")
        block3 = BlockUtils.pull_next_block(app_data, "queue", "queue_idx")
    )");
    
    sol::table block1 = lua["block1"];
    sol::table block2 = lua["block2"];
    sol::table block3 = lua["block3"];
    
    ASSERT_EQ(block1["x"].get<int>(), 0);
    ASSERT_EQ(block2["x"].get<int>(), 64);
    ASSERT_EQ(block3["x"].get<int>(), 0);
    ASSERT_EQ(block3["y"].get<int>(), 64);
}

// テスト15: pull_next_block がキュー末尾でnilを返す
TEST_F(BlockUtilsTest, PullNextBlockReturnsNilWhenEmpty) {
    lua.new_usertype<AppData>("MockAppData",
        "set_string", &AppData::set_string,
        "get_string", &AppData::get_string,
        "pop_next_index", &AppData::pop_next_index
    );
    
    AppData data(100, 100);
    lua["app_data"] = &data;
    
    lua.script(R"(
        local BlockUtils = require("lib.BlockUtils")
        
        local blocks = {
            {x = 0, y = 0, w = 64, h = 64}
        }
        
        BlockUtils.setup_shared_queue(app_data, blocks, "queue2", "queue2_idx")
        
        -- 1つ目は取得できる
        block1 = BlockUtils.pull_next_block(app_data, "queue2", "queue2_idx")
        -- 2つ目はnil
        block2 = BlockUtils.pull_next_block(app_data, "queue2", "queue2_idx")
        
        is_block1_valid = block1 ~= nil
        is_block2_nil = block2 == nil
    )");
    
    bool is_block1_valid = lua["is_block1_valid"];
    bool is_block2_nil = lua["is_block2_nil"];
    
    ASSERT_TRUE(is_block1_valid);
    ASSERT_TRUE(is_block2_nil);
}
