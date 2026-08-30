#include <gtest/gtest.h>
#include <sol/sol.hpp>

class RenderStageTest : public ::testing::Test {
protected:
    void SetUp() override {
        lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::table,
                           sol::lib::coroutine);
        lua.script("package.path = package.path .. ';../../?.lua'");
    }

    sol::state lua;
};

TEST_F(RenderStageTest, StartWorkersCreatesConfiguredWorkerGroup) {
    auto result = lua.safe_script(R"(
        package.loaded["lib.BlockUtils"] = {
            generate_blocks = function(width, height, block_size, overlap)
                return { { x = 0, y = 0, w = width, h = height } }
            end,
            shuffle_blocks = function(blocks) return blocks end,
            setup_shared_queue = function(data, blocks, queue_name)
                data.queue_name = queue_name
                data.block_count = #blocks
            end,
        }

        local starts = {}
        ThreadWorker = {
            create = function(data, scene, x, y, w, h, id)
                return {
                    start = function(self, script_path, scene_type)
                        starts[#starts + 1] = { script_path, scene_type, x, y, w, h, id }
                    end,
                }
            end,
        }

        local RenderStage = require("lib.RenderStage")
        local owner = {
            width = 80, height = 60, BLOCK_SIZE = 16, NUM_THREADS = 3,
            data = {}, scene = {}, current_scene_type = "test_scene",
        }
        local workers = RenderStage.start_workers(owner, "render_queue", "workers/ray_worker.lua")
        return #workers, owner.data.queue_name, owner.data.block_count,
               #starts, starts[1][1], starts[3][2], starts[3][7]
    )");

    ASSERT_TRUE(result.valid()) << sol::error(result).what();
    std::tuple<int, std::string, int, int, std::string, std::string, int> values = result;
    EXPECT_EQ(std::get<0>(values), 3);
    EXPECT_EQ(std::get<1>(values), "render_queue");
    EXPECT_EQ(std::get<2>(values), 1);
    EXPECT_EQ(std::get<3>(values), 3);
    EXPECT_EQ(std::get<4>(values), "workers/ray_worker.lua");
    EXPECT_EQ(std::get<5>(values), "test_scene");
    EXPECT_EQ(std::get<6>(values), 2);
}

TEST_F(RenderStageTest, AllWorkersDoneRequiresEveryWorkerToFinish) {
    auto result = lua.safe_script(R"(
        local RenderStage = require("lib.RenderStage")
        local done = { is_done = function() return true end }
        local pending = { is_done = function() return false end }
        return RenderStage.all_workers_done({ done, done }),
               RenderStage.all_workers_done({ done, pending })
    )");

    ASSERT_TRUE(result.valid()) << sol::error(result).what();
    std::tuple<bool, bool> values = result;
    EXPECT_TRUE(std::get<0>(values));
    EXPECT_FALSE(std::get<1>(values));
}

TEST_F(RenderStageTest, CoroutineProcessesBlocksAndRunsCompletionCallback) {
    auto result = lua.safe_script(R"(
        package.loaded["lib.BlockUtils"] = {
            generate_blocks = function() return { {} } end,
            shuffle_blocks = function(blocks) return blocks end,
            setup_shared_queue = function(data, blocks, queue_name)
                data.queue_name = queue_name
            end,
        }
        package.loaded["workers.worker_utils"] = {
            process_blocks = function(data, queue_name, index_name, callback, check_cancel, _, on_block)
                data.processed_queue = queue_name
                data.index_name = index_name
                callback(data, 4, 5)
                on_block()
            end,
        }

        local RenderStage = require("lib.RenderStage")
        local owner = { width = 10, height = 10, BLOCK_SIZE = 4, data = {} }
        local pixel, completed = "", 0
        local co = RenderStage.create_coroutine(
            owner,
            "posteffect_queue",
            function(data, x, y) pixel = x .. ":" .. y end,
            function() completed = completed + 1 end
        )

        local first_ok = coroutine.resume(co)
        local status_after_block = coroutine.status(co)
        local second_ok = coroutine.resume(co)
        return first_ok, second_ok, status_after_block, coroutine.status(co),
               owner.data.queue_name, owner.data.processed_queue,
               owner.data.index_name, pixel, completed
    )");

    ASSERT_TRUE(result.valid()) << sol::error(result).what();
    std::tuple<bool, bool, std::string, std::string, std::string, std::string,
               std::string, std::string, int> values = result;
    EXPECT_TRUE(std::get<0>(values));
    EXPECT_TRUE(std::get<1>(values));
    EXPECT_EQ(std::get<2>(values), "suspended");
    EXPECT_EQ(std::get<3>(values), "dead");
    EXPECT_EQ(std::get<4>(values), "posteffect_queue");
    EXPECT_EQ(std::get<5>(values), "posteffect_queue");
    EXPECT_EQ(std::get<6>(values), "posteffect_queue_idx");
    EXPECT_EQ(std::get<7>(values), "4:5");
    EXPECT_EQ(std::get<8>(values), 1);
}
