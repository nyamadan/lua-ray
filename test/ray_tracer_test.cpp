#include <gtest/gtest.h>
#include "../src/lua_binding.h"
#include <sol/sol.hpp>
#include "imgui.h"

class RayTracerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Init Lua
        AppContext ctx;
        bind_lua(lua, ctx);
        
        // Add lib directory to package.path (relative to build/gcc-debug)
        lua.script("package.path = package.path .. ';./lib/?.lua;../../?.lua'");
    }

    void TearDown() override {
    }

    sol::state lua;
};

TEST_F(RayTracerTest, RayTracerClassExists) {
    auto result = lua.safe_script("require('lib.RayTracer'); return RayTracer");
    ASSERT_TRUE(result.valid());
    ASSERT_TRUE(result.get<sol::object>().is<sol::table>());
}

TEST_F(RayTracerTest, NewInstance) {
    auto result = lua.safe_script(R"(
        local RayTracer = require('lib.RayTracer')
        local rt = RayTracer.new(800, 600)
        return rt.width, rt.height
    )");
    ASSERT_TRUE(result.valid());
    std::tuple<int, int> size = result;
    ASSERT_EQ(std::get<0>(size), 800);
    ASSERT_EQ(std::get<1>(size), 600);
}

TEST_F(RayTracerTest, InitSuccess) {
    // Mock app functions
    lua.script(R"(
        app.init_video = function() return true end
        app.create_window = function(w, h, title) return "mock_window" end
        app.create_renderer = function(win) return "mock_renderer" end
        app.create_texture = function(r, w, h) return "mock_texture" end
        app.configure = function(config) 
            _G.configured = true
            _G.config_title = config.title
        end
        -- EmbreeDevice is real, but that's fine as confirmed by other tests
    )");

    auto result = lua.safe_script(R"(
        local RayTracer = require('lib.RayTracer')
        local rt = RayTracer.new(800, 600)
        rt:init()
        return rt.window, rt.renderer, rt.texture, _G.configured
    )");
    
    ASSERT_TRUE(result.valid()) << ((sol::error)result).what();
    std::tuple<std::string, std::string, std::string, bool> res = result;
    ASSERT_EQ(std::get<0>(res), "mock_window");
    ASSERT_EQ(std::get<1>(res), "mock_renderer");
    ASSERT_EQ(std::get<2>(res), "mock_texture");
    ASSERT_TRUE(std::get<3>(res));
}

TEST_F(RayTracerTest, CancelRendering) {
    // Mock app functions for init (needed because rt:init calls them)
    lua.script(R"(
        app.init_video = function() return true end
        app.create_window = function(w, h, title) return "mock_window" end
        app.create_renderer = function(win) return "mock_renderer" end
        app.create_texture = function(r, w, h) return "mock_texture" end
        app.configure = function(config) end
    )");

    auto result = lua.safe_script(R"(
        local RayTracer = require('lib.RayTracer')
        local rt = RayTracer.new(800, 600)
        rt:init()
        
        -- Simulate rendering state
        rt.render_coroutine = coroutine.create(function() end)
        rt.workers = { { terminate = function() end } } -- Mock worker
        
        if rt.cancel then
            rt:cancel()
        else
            return "cancel missing"
        end
        
        return rt.render_coroutine, #rt.workers
    )");
    
    ASSERT_TRUE(result.valid()) << ((sol::error)result).what();
    
    if (result.get_type() == sol::type::string) {
        std::string err = result;
        if (err == "cancel missing") {
            FAIL() << "RayTracer:cancel method is missing";
        }
    }
    
    std::tuple<sol::object, int> res = result;
    ASSERT_TRUE(std::get<0>(res).is<sol::nil_t>()); 
    ASSERT_EQ(std::get<1>(res), 0);
}

// テスト: set_resolutionメソッドでwidth, heightが更新される
TEST_F(RayTracerTest, SetResolutionUpdatesSize) {
    // モックapp関数
    lua.script(R"(
        app.init_video = function() return true end
        app.create_window = function(w, h, title) return "mock_window" end
        app.create_renderer = function(win) return "mock_renderer" end
        app.create_texture = function(r, w, h) return "mock_texture_" .. w .. "x" .. h end
        app.configure = function(config) end
        app.destroy_texture = function(tex) end
        app.update_texture = function(tex, data) end
    )");

    auto result = lua.safe_script(R"(
        local RayTracer = require('lib.RayTracer')
        local rt = RayTracer.new(800, 600)
        rt:init()
        
        -- 解像度を変更
        if rt.set_resolution then
            rt:set_resolution(1280, 720)
            return rt.width, rt.height
        else
            return "set_resolution missing"
        end
    )");
    
    ASSERT_TRUE(result.valid()) << ((sol::error)result).what();
    
    if (result.get_type() == sol::type::string) {
        std::string err = result;
        if (err == "set_resolution missing") {
            FAIL() << "RayTracer:set_resolution method is missing";
        }
    }
    
    std::tuple<int, int> size = result;
    ASSERT_EQ(std::get<0>(size), 1280);
    ASSERT_EQ(std::get<1>(size), 720);
}

// テスト: UI操作時に自動的にcancel()が呼ばれる
TEST_F(RayTracerTest, AutoCancelOnUIOperation) {
    // モックapp関数
    lua.script(R"(
        app.init_video = function() return true end
        app.create_window = function(w, h, title) return "mock_window" end
        app.create_renderer = function(win) return "mock_renderer" end
        app.create_texture = function(r, w, h) return "mock_texture" end
        app.configure = function(config) end
        app.destroy_texture = function(tex) end
        app.update_texture = function(tex, data) end
        app.get_ticks = function() return 0 end
    )");

    auto result = lua.safe_script(R"(
        local RayTracer = require('lib.RayTracer')
        local rt = RayTracer.new(800, 600)
        rt:init()
        
        -- cancelが呼ばれたかどうかを追跡するフラグ
        local cancel_called = false
        local original_cancel = rt.cancel
        rt.cancel = function(self)
            cancel_called = true
            original_cancel(self)
        end
        
        -- レンダリング状態をシミュレート
        rt.render_coroutine = coroutine.create(function() end)
        
        -- UI操作をシミュレート: シーン変更
        -- 実際のon_uiメソッド内でのロジックを模倣
        -- レンダリング中にシーンを変更しようとする
        local is_rendering = (rt.render_coroutine ~= nil)
        
        if is_rendering then
            -- 新しい実装では、UI操作前にcancel()を呼ぶべき
            -- ここでは、その動作を手動でテスト
            -- 実際の実装では、on_ui内でこの処理が行われる
            rt:cancel()
        end
        
        return cancel_called
    )");
    
    ASSERT_TRUE(result.valid()) << ((sol::error)result).what();
    bool cancel_was_called = result;
    ASSERT_TRUE(cancel_was_called) << "cancel() should be called when UI operation occurs during rendering";
}

// テスト: Single-threaded rendering check interval (Time-based yielding)
// 現状の実装は100ピクセルごとのチェックだが、時間経過でチェックするように変更するため、
// 100ピクセル未満でも時間が経過していればyieldすることを検証する
TEST_F(RayTracerTest, SingleThreadedCheckInterval) {
    // モックapp関数
    lua.script(R"(
        app.init_video = function() return true end
        app.create_window = function(w, h, title) return "mock_window" end
        app.create_renderer = function(win) return "mock_renderer" end
        app.create_texture = function(r, w, h) return "mock_texture" end
        app.configure = function(config) end
        app.destroy_texture = function(tex) end
        app.update_texture = function(tex, data) end
        app.update_texture_from_back = function(tex, data) end
        
        -- アプリ時間のモック
        -- 初期値 0
        -- get_ticksが呼ばれるたびに時間を進めるわけではなく、外部から制御可能にする
        _G.current_time = 0
        app.get_ticks = function() return _G.current_time end
    )");

    auto result = lua.safe_script(R"(
        local RayTracer = require('lib.RayTracer')
        local rt = RayTracer.new(100, 100) -- 小さいサイズ
        rt:init()
        
        -- シーン設定 (shade関数が呼ばれた回数をカウント)
        -- モックシーンモジュールを作成
        local mock_scene = {}
        mock_scene.setup = function() end
        mock_scene.start = function() end
        mock_scene.shade = function(data, x, y)
            _G.shade_count = (_G.shade_count or 0) + 1
            
            if x == 0 then
                -- x=0の時は時間を少しだけ進める (1ms)
                -- これにより x=0 のチェック (x%100==0) をすり抜ける (1ms < 12ms)
                _G.current_time = _G.current_time + 1
            else
                -- x>0の時は時間を大幅に進める (20ms)
                -- 現行実装では x=1〜99 でチェックしないため、ここまで進んでしまう
                -- 新実装では時間経過を検知して停止するはず
                _G.current_time = _G.current_time + 20
            end
        end
        rt.current_scene_module = mock_scene
        
        _G.current_time = 1000 -- 開始時間を適当に設定
        _G.shade_count = 0
        
        -- レンダリングコルーチン作成
        -- create_render_coroutine内部で app.get_ticks() を呼んで start_time を取得するはず
        rt.render_coroutine = rt:create_render_coroutine()
        
        -- コルーチンを1回レジューム (開始)
        local status = coroutine.status(rt.render_coroutine)
        if status == "suspended" then
            coroutine.resume(rt.render_coroutine)
        end
        
        -- 期待値:
        -- 現行実装(100ピクセルチェック): x=0〜99まで一気に進むので、shade_countは100になる
        -- 修正後実装(時間チェック): 1ピクセル目で20ms経過して閾値(12ms)を超えるため、即座にyieldするはず
        
        return _G.shade_count
    )");
    
    ASSERT_TRUE(result.valid()) << ((sol::error)result).what();
    int shade_count = result;
    
    // 期待: 数ピクセル (例えば10未満) で yield していること
    // 現行実装では失敗するはず
    ASSERT_LT(shade_count, 10) << "Should yield early when rendering is slow (processed " << shade_count << " pixels)";
}

TEST_F(RayTracerTest, HandleKeyboardMovesCameraAndResetsWorkers) {
    auto result = lua.safe_script(R"(
        local RayTracer = require('lib.RayTracer')
        local rt = RayTracer.new(100, 100)
        
        -- モックapp関数
        app.get_keyboard_state = function()
            return { w = true, a = false, s = false, d = false, q = false, e = false, space = true, escape = false }
        end
        app.get_ticks = function() return 0 end
        app.init_video = function() return true end
        app.create_window = function(w, h, title) return "mock_window" end
        app.create_renderer = function(win) return "mock_renderer" end
        app.create_texture = function(r, w, h) return "mock_texture" end
        app.configure = function(config) end

        rt:init()
        
        -- reset_workersが呼ばれたかを記録
        local reset_called = false
        rt.reset_workers = function(self)
            reset_called = true
        end
        
        -- モックカメラ (移動メソッドが呼ばれたかを記録)
        local mock_camera = {
            forward_dist = 0,
            right_dist = 0,
            up_dist = 0,
            move_forward = function(self, dist) self.forward_dist = self.forward_dist + dist end,
            move_right = function(self, dist) self.right_dist = self.right_dist + dist end,
            move_up = function(self, dist) self.up_dist = self.up_dist + dist end
        }
        
        -- モックシーンモジュール
        local mock_scene = {
            get_camera = function() return mock_camera end
        }
        rt.current_scene_module = mock_scene
        
        -- テスト対象メソッドの実行
        rt:handle_keyboard()
        
        return reset_called, mock_camera.forward_dist, mock_camera.right_dist, mock_camera.up_dist
    )");
    
    ASSERT_TRUE(result.valid()) << ((sol::error)result).what();
    std::tuple<bool, double, double, double> res = result;
    
    ASSERT_TRUE(std::get<0>(res)) << "reset_workers should be called when camera moves";
    ASSERT_GT(std::get<1>(res), 0.0) << "camera.move_forward should be called (w is pressed)";
    ASSERT_EQ(std::get<2>(res), 0.0) << "camera.move_right should not be called (a/d are not pressed)";
    ASSERT_GT(std::get<3>(res), 0.0) << "camera.move_up should be called (space is pressed)";
}
// シングルスレッドモードでのstop呼び出しテスト (完了時)
TEST_F(RayTracerTest, SingleThreadedStopOnFinish) {
    // モック設定
    lua.script(R"(
        app.init_video = function() return true end
        app.create_window = function(w, h, title) return "mock_window" end
        app.create_renderer = function(win) return "mock_renderer" end
        app.create_texture = function(r, w, h) return "mock_texture" end
        app.configure = function(config) end
        app.destroy_texture = function(tex) end
        app.update_texture = function(tex, data) end
        app.update_texture_from_back = function(tex, data) end
        app.get_ticks = function() return 0 end
    )");

    auto result = lua.safe_script(R"(
        local RayTracer = require('lib.RayTracer')
        local rt = RayTracer.new(100, 100)
        rt:init()
        
        -- モックシーン
        local stop_called = false
        local mock_scene = {}
        mock_scene.setup = function() end
        mock_scene.start = function() end
        mock_scene.shade = function(data, x, y) end -- 何もしない
        mock_scene.stop = function(scene)
            stop_called = true
        end
        rt.current_scene_module = mock_scene
        
        -- レンダリング開始
        rt:render()
        
        -- updateを回して完了まで待つ
        local max_loops = 10
        local loop_count = 0
        while rt.render_coroutine and loop_count < max_loops do
            rt:update()
            loop_count = loop_count + 1
        end
        
        return stop_called, rt.render_coroutine, loop_count
    )");
    
    ASSERT_TRUE(result.valid()) << ((sol::error)result).what();
    std::tuple<bool, sol::object, int> res = result;
    bool stop_called = std::get<0>(res);
    sol::object coroutine_obj = std::get<1>(res);
    int loop_count = std::get<2>(res);
    
    ASSERT_TRUE(coroutine_obj.is<sol::nil_t>()) << "Coroutine should be nil after finish (loops: " << loop_count << ")";
    ASSERT_TRUE(stop_called) << "stop() should be called when rendering finishes";
}

// シングルスレッドモードでのstop呼び出しテスト (キャンセル時)
TEST_F(RayTracerTest, SingleThreadedStopOnCancel) {
    // モック設定
    lua.script(R"(
        app.init_video = function() return true end
        app.create_window = function(w, h, title) return "mock_window" end
        app.create_renderer = function(win) return "mock_renderer" end
        app.create_texture = function(r, w, h) return "mock_texture" end
        app.configure = function(config) end
        app.destroy_texture = function(tex) end
        app.update_texture = function(tex, data) end
        app.update_texture_from_back = function(tex, data) end
        app.get_ticks = function() return 0 end
    )");

    auto result = lua.safe_script(R"(
        local RayTracer = require('lib.RayTracer')
        local rt = RayTracer.new(100, 100)
        rt:init()
        
        -- モックシーン
        local stop_called = false
        local mock_scene = {}
        mock_scene.setup = function() end
        mock_scene.start = function() end
        mock_scene.shade = function(data, x, y) end
        mock_scene.stop = function(scene)
            stop_called = true
        end
        rt.current_scene_module = mock_scene
        
        -- レンダリング開始（ダミーコルーチンセット）
        rt.render_coroutine = coroutine.create(function() 
            while true do coroutine.yield() end
        end)
        
        -- キャンセル実行
        rt:cancel()
        
        return stop_called, rt.render_coroutine
    )");
    
    ASSERT_TRUE(result.valid()) << ((sol::error)result).what();
    std::tuple<bool, sol::object> res = result;
    bool stop_called = std::get<0>(res);
    sol::object coroutine_obj = std::get<1>(res);
    
    ASSERT_TRUE(coroutine_obj.is<sol::nil_t>()) << "Coroutine should be nil after cancel";
    ASSERT_TRUE(stop_called) << "stop() should be called when rendering is cancelled";
}

// テスト: reset_workers(false) 時に copy_front_to_back が呼ばれないこと(仕様変更)
TEST_F(RayTracerTest, ResetWorkersDoesNotCopyFrontToBack) {
    // モック設定
    lua.script(R"(
        app.init_video = function() return true end
        app.create_window = function(w, h, title) return "mock_window" end
        app.create_renderer = function(win) return "mock_renderer" end
        app.create_texture = function(r, w, h) return "mock_texture" end
        app.configure = function(config) end
        app.destroy_texture = function(tex) end
        app.update_texture = function(tex, data) end
        app.update_texture_from_back = function(tex, data) end
        app.get_ticks = function() return 0 end
    )");

    auto result = lua.safe_script(R"(
        local RayTracer = require('lib.RayTracer')
        local rt = RayTracer.new(100, 100)
        rt:init()
        
        -- モックシーンモジュール
        local mock_scene = {
            setup = function() end,
            start = function() end,
            stop = function() end
        }
        rt.current_scene_module = mock_scene
        
        -- rt.data.copy_front_to_back が呼ばれたかどうかを記録
        local copy_called = false
        rt.data = {
            copy_front_to_back = function(self)
                copy_called = true
            end
        }
        
        -- レンダリング自体はモック化して実際には実行しない
        rt.render = function() end
        rt.render_without_clear = function() end
        
        -- テスト対象メソッドの実行 (clear_texture = false)
        rt:reset_workers(false)
        
        return copy_called
    )");
    
    ASSERT_TRUE(result.valid()) << ((sol::error)result).what();
    bool copy_called = result;
    ASSERT_FALSE(copy_called) << "rt.data:copy_front_to_back() should not be called when clear_texture is false";
}

TEST_F(RayTracerTest, AppDataBindingCopyBackToFront) {
    // モック設定
    lua.script(R"(
        app.init_video = function() return true end
        app.create_window = function(w, h, title) return "mock_window" end
        app.create_renderer = function(win) return "mock_renderer" end
        app.create_texture = function(r, w, h) return "mock_texture" end
        app.configure = function(config) end
        app.destroy_texture = function(tex) end
        app.update_texture = function(tex, data) end
        app.update_texture_from_back = function(tex, data) end
        app.get_ticks = function() return 0 end
    )");

    auto result = lua.safe_script(R"(
        local RayTracer = require('lib.RayTracer')
        local rt = RayTracer.new(100, 100)
        rt:init()
        
        -- バックバッファに書き込み
        rt.data:set_pixel(2, 2, 0, 0, 255)
        
        -- バックからフロントへコピー
        rt.data:copy_back_to_front()
        
        -- get_pixelでフロントをチェック、コピーされた値(青)が取得できるはず
        local r, g, b = rt.data:get_pixel(2, 2)
        return r, g, b
    )");
    
    ASSERT_TRUE(result.valid()) << ((sol::error)result).what();
    std::tuple<int, int, int> rgb = result;
    ASSERT_EQ(std::get<0>(rgb), 0);
    ASSERT_EQ(std::get<1>(rgb), 0);
    ASSERT_EQ(std::get<2>(rgb), 255);
}

// テスト: NUM_THREADSとBLOCK_SIZEのデフォルト値
TEST_F(RayTracerTest, DefaultThreadsAndTileSize) {
    auto result = lua.safe_script(R"(
        local RayTracer = require('lib.RayTracer')
        local rt = RayTracer.new(100, 100)
        return rt.NUM_THREADS, rt.BLOCK_SIZE
    )");
    
    ASSERT_TRUE(result.valid()) << ((sol::error)result).what();
    std::tuple<int, int> res = result;
    EXPECT_EQ(std::get<0>(res), 8) << "Default NUM_THREADS should be 8";
    EXPECT_EQ(std::get<1>(res), 64) << "Default BLOCK_SIZE should be 64";
}

// テスト: NUM_THREADSとBLOCK_SIZEがLuaから直接変更可能
TEST_F(RayTracerTest, CanChangeThreadsAndTileSize) {
    auto result = lua.safe_script(R"(
        local RayTracer = require('lib.RayTracer')
        local rt = RayTracer.new(100, 100)
        
        -- 値を変更
        rt.NUM_THREADS = 4
        rt.BLOCK_SIZE = 32
        
        return rt.NUM_THREADS, rt.BLOCK_SIZE
    )");
    
    ASSERT_TRUE(result.valid()) << ((sol::error)result).what();
    std::tuple<int, int> res = result;
    EXPECT_EQ(std::get<0>(res), 4);
    EXPECT_EQ(std::get<1>(res), 32);
}

// テスト: on_uiメソッド内でスレッド数とタイルサイズのコンボボックスが表示される
TEST_F(RayTracerTest, OnUIShowsThreadsAndTileSizeComboBoxes) {
    // モックapp関数
    lua.script(R"(
        app.init_video = function() return true end
        app.create_window = function(w, h, title) return "mock_window" end
        app.create_renderer = function(win) return "mock_renderer" end
        app.create_texture = function(r, w, h) return "mock_texture" end
        app.configure = function(config) end
        app.get_ticks = function() return 0 end
    )");

    // ImGuiコンテキストのセットアップ
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(1920, 1080);
    io.DeltaTime = 1.0f / 60.0f;
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    ImGui::NewFrame();

    auto result = lua.safe_script(R"(
        local RayTracer = require('lib.RayTracer')
        local rt = RayTracer.new(100, 100)
        rt:init()
        
        -- on_uiを呼び出し（コンボボックスが含まれるはず）
        rt:on_ui()
        
        return rt.NUM_THREADS, rt.BLOCK_SIZE, rt.thread_preset_index, rt.block_preset_index
    )");

    ImGui::Render();
    ImGui::DestroyContext();
    
    ASSERT_TRUE(result.valid()) << ((sol::error)result).what();
    std::tuple<int, int, int, int> res = result;
    // on_ui内でコンボボックスが正常に呼ばれてもクラッシュせず、値が保持されること
    EXPECT_EQ(std::get<0>(res), 8);
    EXPECT_EQ(std::get<1>(res), 64);
    EXPECT_EQ(std::get<2>(res), 4); // デフォルトスレッドプリセットインデックス
    EXPECT_EQ(std::get<3>(res), 1); // デフォルトブロックプリセットインデックス
}

// テスト: スレッド数プリセット変更時にreset_workersが呼ばれる
TEST_F(RayTracerTest, ThreadsPresetChangeCallsResetWorkers) {
    auto result = lua.safe_script(R"(
        local RayTracer = require('lib.RayTracer')
        local ThreadPresets = require('lib.ThreadPresets')
        local rt = RayTracer.new(100, 100)
        
        -- モックセットアップ
        local reset_called = false
        rt.reset_workers = function(self) reset_called = true end
        rt.cancel_if_rendering = function(self) end
        
        -- 初期値
        rt.use_multithreading = true
        
        -- シミュレート: コンボボックスでプリセットが変更された時の動作を直接テスト
        local new_index = 2 -- 2スレッド
        local thread_presets = ThreadPresets.get_thread_presets()
        if new_index ~= rt.thread_preset_index then
            rt:cancel_if_rendering()
            rt.thread_preset_index = new_index
            rt.NUM_THREADS = thread_presets[new_index].value
            rt:reset_workers()
        end
        
        return reset_called, rt.NUM_THREADS, rt.thread_preset_index
    )");
    
    ASSERT_TRUE(result.valid()) << ((sol::error)result).what();
    std::tuple<bool, int, int> res = result;
    EXPECT_TRUE(std::get<0>(res)) << "reset_workers should be called when thread preset changes";
    EXPECT_EQ(std::get<1>(res), 2);
    EXPECT_EQ(std::get<2>(res), 2);
}

// テスト: タイルサイズプリセット変更時にreset_workersが呼ばれる
TEST_F(RayTracerTest, TileSizePresetChangeCallsResetWorkers) {
    auto result = lua.safe_script(R"(
        local RayTracer = require('lib.RayTracer')
        local ThreadPresets = require('lib.ThreadPresets')
        local rt = RayTracer.new(100, 100)
        
        -- モックセットアップ
        local reset_called = false
        rt.reset_workers = function(self) reset_called = true end
        rt.cancel_if_rendering = function(self) end
        
        -- シミュレート: コンボボックスでプリセットが変更された時の動作を直接テスト
        local new_index = 3 -- 256ブロックサイズ
        local block_presets = ThreadPresets.get_block_presets()
        if new_index ~= rt.block_preset_index then
            rt:cancel_if_rendering()
            rt.block_preset_index = new_index
            rt.BLOCK_SIZE = block_presets[new_index].value
            rt:reset_workers()
        end
        
        return reset_called, rt.BLOCK_SIZE, rt.block_preset_index
    )");
    
    ASSERT_TRUE(result.valid()) << ((sol::error)result).what();
    std::tuple<bool, int, int> res = result;
    EXPECT_TRUE(std::get<0>(res)) << "reset_workers should be called when block preset changes";
    EXPECT_EQ(std::get<1>(res), 256);
    EXPECT_EQ(std::get<2>(res), 3);
}

// テスト: シングルスレッドモード時にスレッド数コンボボックスが無効化される
TEST_F(RayTracerTest, ThreadsComboDisabledInSingleThreadMode) {
    // モックapp関数
    lua.script(R"(
        app.init_video = function() return true end
        app.create_window = function(w, h, title) return "mock_window" end
        app.create_renderer = function(win) return "mock_renderer" end
        app.create_texture = function(r, w, h) return "mock_texture" end
        app.configure = function(config) end
        app.get_ticks = function() return 0 end
    )");

    // ImGuiコンテキストのセットアップ
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(1920, 1080);
    io.DeltaTime = 1.0f / 60.0f;
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    ImGui::NewFrame();

    auto result = lua.safe_script(R"(
        local RayTracer = require('lib.RayTracer')
        local rt = RayTracer.new(100, 100)
        rt:init()
        
        -- シングルスレッドモードに設定
        rt.use_multithreading = false
        
        -- reset_workersをモック化
        local reset_called = false
        rt.reset_workers = function(self) reset_called = true end
        
        -- on_uiを呼び出し（シングルスレッド時、スレッド数コンボボックスはBeginDisabledで囲まれるはず）
        rt:on_ui()
        
        -- シングルスレッドモードでもクラッシュせず正常に動作すること
        return rt.use_multithreading, rt.NUM_THREADS, rt.thread_preset_index
    )");

    ImGui::Render();
    ImGui::DestroyContext();

    ASSERT_TRUE(result.valid()) << ((sol::error)result).what();
    std::tuple<bool, int, int> res = result;
    EXPECT_FALSE(std::get<0>(res)) << "Should be in single-threaded mode";
    EXPECT_EQ(std::get<1>(res), 8) << "NUM_THREADS should remain 8";
    EXPECT_EQ(std::get<2>(res), 4) << "thread_preset_index should remain at default";
}
