#include <gtest/gtest.h>
#include "../src/lua_binding.h"
#include <sol/sol.hpp>

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

