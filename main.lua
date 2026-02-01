-- SDL3 Ray Tracing in Lua

-- RayTracer Class Definition
RayTracer = {}
RayTracer.__index = RayTracer

function RayTracer.new(width, height)
    local self = setmetatable({}, RayTracer)
    self.width = width
    self.height = height
    self.window = nil
    self.renderer = nil
    self.texture = nil
    self.device = nil
    self.scene = nil
    self.current_scene_type = "color_pattern" -- Default scene
    self.current_scene_module = nil -- モジュールはreset_sceneで読み込まれる
    self.render_coroutine = nil -- レンダリング用コルーチン
    self.render_progress = 0.0 -- レンダリング進行状況 (0.0〜1.0)
    return self
end

function RayTracer:init()
    -- 1. Init Video
    if not app.init_video() then
        error("Error: Failed to init video")
    end

    -- 2. Create Window
    self.window = app.create_window(self.width, self.height, "Lua Ray Tracing")
    if self.window == nil then
        error("Error: Failed to create window")
    end

    -- 3. Create Renderer
    self.renderer = app.create_renderer(self.window)
    if self.renderer == nil then
        error("Error: Failed to create renderer")
    end

    -- 4. Create Texture
    self.texture = app.create_texture(self.renderer, self.width, self.height)
    if self.texture == nil then
        error("Error: Failed to create texture")
    end

    -- 5. Create AppData
    self.data = AppData.new(self.width, self.height)

    -- 6. Configure App (Dependency Injection for C++)
    app.configure({
        width = self.width,
        height = self.height,
        title = "Lua Ray Tracing (AppData)",
        window = self.window,
        renderer = self.renderer,
        texture = self.texture
    })
    
    -- Initialize Embree Device once
    print("Initializing Embree Device...")
    self.device = EmbreeDevice.new()
end

function RayTracer:reset_scene(scene_type, force_reload)
    print("Resetting scene to: " .. scene_type)
    
    -- クリーンアップコールバックの呼び出し
    if self.current_scene_module and self.current_scene_module.cleanup then
        print("Calling cleanup callback for current scene module")
        self.current_scene_module.cleanup(self.scene)
    end
    
    -- Explicitly release the old scene resources
    if self.scene then
        self.scene:release()
    end

    -- Clear current scene reference (Lua GC will handle the C++ object destruction eventually, but we released resources manually)
    self.scene = nil
    collectgarbage() -- Optional: Suggest GC to run
    
    -- Create new empty scene
    self.scene = self.device:create_scene()
    
    self.current_scene_type = scene_type
    
    -- シーンモジュールの動的読み込み(強制再読み込みオプション付き)
    local scene_module_path = "scenes." .. scene_type
    
    -- 強制再読み込みが指定されている場合、package.loadedからモジュールを削除
    if force_reload then
        print("Force reloading module: " .. scene_module_path)
        package.loaded[scene_module_path] = nil
    end
    
    local success, scene_module = pcall(require, scene_module_path)
    
    if success and scene_module then
        self.current_scene_module = scene_module
    else
        print("Unknown scene type or failed to load: " .. scene_type .. ", defaulting to color_pattern")
        -- デフォルトのcolor_patternも強制再読み込み
        if force_reload then
            package.loaded["scenes.color_pattern"] = nil
        end
        self.current_scene_module = require("scenes.color_pattern")
    end
    
    -- シーンのセットアップとカメラの初期化
    self.current_scene_module.setup(self.scene, self.data)
    
    self.scene:commit()
    -- Re-render immediately after switch
    self:render()
end

function RayTracer:init_scene()
    self:reset_scene(self.current_scene_type)
end

function RayTracer:update_texture()
    -- Update Texture from AppData
    app.update_texture(self.texture, self.data)
end

-- レンダリングコルーチンを作成
function RayTracer:create_render_coroutine()
    return coroutine.create(function()
        local start_time = os.clock()
        local time_limit = 0.01  -- 10ms
        local total_pixels = self.width * self.height
        local rendered_pixels = 0
        
        for y = 0, self.height - 1 do
            for x = 0, self.width - 1 do
                -- シーンモジュールのshade関数を使用してピクセルを描画
                self.current_scene_module.shade(self.data, x, y)
                rendered_pixels = rendered_pixels + 1
                
                -- 時間チェック
                local elapsed = os.clock() - start_time
                if elapsed >= time_limit then
                    local progress = rendered_pixels / total_pixels
                    coroutine.yield(progress)  -- 進行状況を返して次のフレームへ
                    start_time = os.clock()  -- 時間をリセット
                end
            end
        end
        
        -- レンダリング完了
        return 1.0  -- 100%完了
    end)
end

-- 毎フレーム呼ばれる更新処理
function RayTracer:update()
    if self.render_coroutine then
        local status = coroutine.status(self.render_coroutine)
        if status == "suspended" then
            local ok, progress = coroutine.resume(self.render_coroutine)
            if not ok then
                print("Coroutine error: " .. tostring(progress))
                self.render_coroutine = nil
                self.render_progress = 0.0
            elseif coroutine.status(self.render_coroutine) == "dead" then
                -- レンダリング完了
                print("Lua render finished.")
                self:update_texture()
                self.render_coroutine = nil
                self.render_progress = 1.0
            else
                -- 進行状況を更新
                self.render_progress = progress or 0.0
                -- レンダリング中、テクスチャを更新
                self:update_texture()
            end
        end
    end
end

function RayTracer:render()
    print("Starting render...")
    self.render_coroutine = self:create_render_coroutine()
end

function RayTracer:on_ui()
    if ImGui.Begin("Lua Ray Tracer Control") then
        ImGui.Text("Welcome to Lua Ray Tracer!")
        ImGui.Text(string.format("Resolution: %d x %d", self.width, self.height))
        
        ImGui.Separator()
        
        ImGui.Text("Scene Selection:")
        local changed = false
        local type = self.current_scene_type
        
        if ImGui.RadioButton("Color Pattern", type == "color_pattern") then
            if type ~= "color_pattern" then
                self:reset_scene("color_pattern")
            end
        end
        ImGui.SameLine()
        if ImGui.RadioButton("Triangle", type == "triangle") then
            if type ~= "triangle" then
                self:reset_scene("triangle")
            end
        end
        ImGui.SameLine()
        if ImGui.RadioButton("Sphere", type == "sphere") then
            if type ~= "sphere" then
                self:reset_scene("sphere")
            end
        end
        
        ImGui.Separator()
        
        if ImGui.Button("Reload Scene") then
            print("Reloading scene module and re-rendering")
            self:reset_scene(self.current_scene_type, true) -- force_reload = true
        end
        
        ImGui.Separator()
        
        -- レンダリング進行状況の表示
        ImGui.Text("Render Progress:")
        local progress_text = string.format("%.1f%%", self.render_progress * 100)
        ImGui.ProgressBar(self.render_progress, progress_text)
        
        if self.render_coroutine then
            ImGui.Text("Status: Rendering...")
        else
            ImGui.Text("Status: Idle")
        end
    end
    ImGui.End()
end


-- Main Execution Flow
local WIDTH = 800
local HEIGHT = 600

-- Instantiate
raytracer = RayTracer.new(WIDTH, HEIGHT)

-- Initialize resources
raytracer:init()

-- Initialize scene
raytracer:init_scene()

-- Initial Render is called inside init_scene -> reset_scene, but we can call it here if we want strictly explicit
-- raytracer:render() -- reset_scene already calls render

-- Set global frame callback
function app.on_frame()
    raytracer:update()
    raytracer:on_ui()
end
