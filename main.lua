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
    self.workers = {} -- Array of ThreadWorker
    self.render_coroutine = nil -- Coroutine for single-threaded rendering
    self.use_multithreading = false -- マルチスレッド使用フラグ
    self.NUM_THREADS = 8 -- スレッド数
    self.render_start_time = 0 -- Rendering start time
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
    
    -- Stop any existing coroutine
    self.render_coroutine = nil
    
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
    -- setup: Geometry creation (Main thread only, once)
    if self.current_scene_module.setup then
        self.current_scene_module.setup(self.scene, self.data)
    end

    -- start: Camera and local state initialization (Every time scene is reset or thread starts)
    if self.current_scene_module.start then
        self.current_scene_module.start(self.scene, self.data)
    else
        print("Warning: Scene module " .. self.current_scene_type .. " missing start function")
    end
    
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

-- スレッドレンダリングを開始
function RayTracer:start_render_threads()
    -- Stop any existing coroutine
    self.render_coroutine = nil
    
    -- 既存のワーカーをクリア (GCでjoinされるが、明示的に待つほうが安全かもしれない)
    self.workers = {}
    
    local h_step = math.ceil(self.height / self.NUM_THREADS)
    
    for i = 0, self.NUM_THREADS - 1 do
        local y_start = i * h_step
        local h = h_step
        if y_start + h > self.height then
            h = self.height - y_start
        end
        
        if h > 0 then
            -- ThreadWorker.create(data, scene, x, y, w, h, id)
            local worker = ThreadWorker.create(self.data, self.scene, 0, y_start, self.width, h, i)
            worker:start("worker.lua", self.current_scene_type)
            table.insert(self.workers, worker)
        end
    end
end

-- シングルスレッドレンダリング用コルーチン作成
function RayTracer:create_render_coroutine()
    return coroutine.create(function()
        print("Starting single-threaded render (Coroutine)...")
        local startTime = app.get_ticks()
        local frameAllowance = 12 -- 12ms
        
        for y = 0, self.height - 1 do
            for x = 0, self.width - 1 do
                self.current_scene_module.shade(self.data, x, y)
                
                -- Check time every 100 pixels to avoid excessive app.get_ticks calls
                if x % 100 == 0 then
                     if (app.get_ticks() - startTime) >= frameAllowance then
                        coroutine.yield()
                        startTime = app.get_ticks() -- Reset start time for next slice
                     end
                end
            end
        end
        
        print(string.format("Single-threaded render finished internally. (Total check in update)"))
    end)
end

-- 毎フレーム呼ばれる更新処理
function RayTracer:update()
    -- Multi-threaded update
    if #self.workers > 0 then
        local all_done = true
        
        for _, worker in ipairs(self.workers) do
            if not worker:is_done() then
                all_done = false
            end
        end
        
        -- レンダリング中、テクスチャを更新
        self:update_texture()
        
        if all_done then
            local end_time = app.get_ticks()
            print(string.format("Lua render finished (Multi-threaded). Time: %d ms", end_time - self.render_start_time))
            self.workers = {} -- 完了
            self:update_texture() -- 最後に一度更新
        end
    
    -- Single-threaded coroutine update
    elseif self.render_coroutine then
        local status = coroutine.status(self.render_coroutine)
        if status == "suspended" then
            local success, err = coroutine.resume(self.render_coroutine)
            if not success then
                print("Coroutine error: " .. tostring(err))
                self.render_coroutine = nil
            end
            self:update_texture()
        elseif status == "dead" then
            local end_time = app.get_ticks()
            print(string.format("Single-threaded render finished. Time: %d ms", end_time - self.render_start_time))
            self.render_coroutine = nil
            self:update_texture()
        end
    end
end

function RayTracer:render()
    print("Starting render...")
    self.render_start_time = app.get_ticks()
    
    -- Clear previous render data
    self.data:clear()
    self:update_texture()

    if self.use_multithreading then
        self:start_render_threads()
    else
        self.render_coroutine = self:create_render_coroutine()
    end
end

function RayTracer:on_ui()
    if ImGui.Begin("Lua Ray Tracer Control") then
        ImGui.Text("Welcome to Lua Ray Tracer!")
        ImGui.Text(string.format("Resolution: %d x %d", self.width, self.height))
        
        ImGui.Separator()

        local is_rendering = (#self.workers > 0) or (self.render_coroutine ~= nil)
        ImGui.BeginDisabled(is_rendering)

        -- Render Mode Selection
        ImGui.Text("Render Mode:")
        if ImGui.RadioButton("Single-threaded", not self.use_multithreading) then
            if self.use_multithreading then
                self.use_multithreading = false
                self:render()
            end
        end
        ImGui.SameLine()
        if ImGui.RadioButton("Multi-threaded", self.use_multithreading) then
            if not self.use_multithreading then
                self.use_multithreading = true
                self:render()
            end
        end

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
        
        ImGui.EndDisabled()
        
        ImGui.Separator()
        
        if #self.workers > 0 then
            ImGui.Text(string.format("Status: Rendering... (%d threads)", #self.workers))
        elseif self.render_coroutine then
            ImGui.Text("Status: Rendering... (Single-threaded)")
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
