// block_utils_test.cpp
// TDD Red Phase: BlockUtils Luaモジュールのテスト

#include <gtest/gtest.h>
#include <sol/sol.hpp>

class BlockUtilsTest : public ::testing::Test {
protected:
    void SetUp() override {
        lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::math, sol::lib::table);
        
        // Luaパスを設定（ビルドディレクトリからの相対パス）
        // gcc-debug/lua-ray-tests.exe から見たパス
        lua.script("package.path = package.path .. ';./lib/?.lua;../../lib/?.lua;../../?.lua'");
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
