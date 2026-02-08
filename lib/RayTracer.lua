-- RayTracer Class Definition
local BlockUtils = require("lib.BlockUtils")
local ResolutionPresets = require("lib.ResolutionPresets")

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
    self.posteffect_workers = {} -- Array of ThreadWorker for PostEffect
    self.render_coroutine = nil -- Coroutine for single-threaded rendering
    self.posteffect_coroutine = nil -- Coroutine for single-threaded PostEffect
    self.use_multithreading = false -- マルチスレッド使用フラグ
    self.NUM_THREADS = 8 -- スレッド数
    self.BLOCK_SIZE = 64 -- ブロックサイズ
    self.render_start_time = 0 -- Rendering start time
    self.current_preset_index = ResolutionPresets.get_default_index() -- 解像度プリセットインデックス
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

-- 解像度を変更
-- テクスチャとAppDataを再作成し、シーンを再描画
function RayTracer:set_resolution(new_width, new_height)
    print(string.format("Changing resolution: %dx%d -> %dx%d", 
        self.width, self.height, new_width, new_height))
    
    -- 実行中のワーカーを停止
    self:terminate_workers()
    
    -- コルーチンを停止
    self.render_coroutine = nil
    self.posteffect_coroutine = nil
    
    -- 古いテクスチャを破棄 (存在する場合)
    if self.texture and app.destroy_texture then
        app.destroy_texture(self.texture)
    end
    
    -- サイズを更新
    self.width = new_width
    self.height = new_height
    
    -- 新しいテクスチャを作成
    self.texture = app.create_texture(self.renderer, self.width, self.height)
    if self.texture == nil then
        error("Error: Failed to create texture for new resolution")
    end
    
    -- 新しいAppDataを作成
    self.data = AppData.new(self.width, self.height)
    
    -- Configure Appを更新
    app.configure({
        width = self.width,
        height = self.height,
        title = "Lua Ray Tracing (AppData)",
        window = self.window,
        renderer = self.renderer,
        texture = self.texture
    })
    
    -- シーンを再初期化（geometryは再利用、カメラなどをリセット）
    if self.current_scene_module and self.current_scene_module.start then
        self.current_scene_module.start(self.scene, self.data)
    end
    
    -- 再レンダリング
    self:render()
end

function RayTracer:reset_scene(scene_type, force_reload)
    print("Resetting scene to: " .. scene_type)
    
    -- 実行中のワーカーを安全に停止
    self:terminate_workers()
    
    -- Stop any existing coroutine
    self.render_coroutine = nil
    self.posteffect_coroutine = nil
    
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
    -- Update Texture from AppData (フロントバッファから)
    app.update_texture(self.texture, self.data)
end

function RayTracer:update_texture_from_back()
    -- Update Texture from AppData's back buffer (レンダリング中途中表示用)
    app.update_texture_from_back(self.texture, self.data)
end

-- すべてのワーカーを安全に停止
function RayTracer:terminate_workers()
    -- レンダリングワーカーをterminate
    for _, worker in ipairs(self.workers) do
        worker:terminate()
    end
    self.workers = {}
    
    -- PostEffectワーカーをterminate
    for _, worker in ipairs(self.posteffect_workers) do
        worker:terminate()
    end
    self.posteffect_workers = {}
end

-- レンダリングをキャンセル
function RayTracer:cancel()
    print("Cancelling rendering...")
    
    -- ワーカーを停止
    self:terminate_workers()
    
    -- コルーチンを破棄
    self.render_coroutine = nil
    self.posteffect_coroutine = nil
    
    -- 状態をアイドルに戻すための追加処理があれば記述
    -- 例: プログレスバーのリセットなど
end

-- スレッドレンダリングを開始（ブロック単位分割、9スレッド制限）
function RayTracer:start_render_threads()
    local json = require("lib.json")
    
    -- Stop any existing coroutine
    self.render_coroutine = nil
    
    -- 既存のワーカーをクリア
    self.workers = {}
    
    -- ブロック単位で画面を分割
    local blocks = BlockUtils.generate_blocks(
        self.width, self.height, self.BLOCK_SIZE, self.NUM_THREADS
    )
    
    -- スレッドIDごとにブロックをグループ化
    local groups = BlockUtils.group_blocks_by_thread(blocks, self.NUM_THREADS)
    
    -- 各スレッドに対してワーカーを起動（NUM_THREADS個のみ）
    for thread_id = 0, self.NUM_THREADS - 1 do
        local thread_blocks = groups[thread_id]
        if #thread_blocks > 0 then
            -- ブロックリストをJSONでAppDataに保存
            local blocks_key = "worker_blocks_" .. thread_id
            self.data:set_string(blocks_key, json.encode(thread_blocks))
            
            -- 最初のブロックをboundsとして渡す（後方互換用）
            local first_block = thread_blocks[1]
            local worker = ThreadWorker.create(
                self.data, self.scene,
                first_block.x, first_block.y, first_block.w, first_block.h,
                thread_id
            )
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
        
        -- レンダリング中、バックバッファからテクスチャを更新
        self:update_texture_from_back()
        
        if all_done then
            local end_time = app.get_ticks()
            print(string.format("Lua render finished (Multi-threaded). Time: %d ms", end_time - self.render_start_time))
            self.workers = {} -- 完了
            
            -- PostEffectが存在する場合は開始
            if self.current_scene_module.post_effect then
                self:start_posteffect()
            else
                -- PostEffect無しの場合はswapしてフロントに反映
                self.data:swap()
                self:update_texture()
            end
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
            self:update_texture_from_back()
        elseif status == "dead" then
            local end_time = app.get_ticks()
            print(string.format("Single-threaded render finished. Time: %d ms", end_time - self.render_start_time))
            self.render_coroutine = nil
            
            -- PostEffectが存在する場合は開始
            if self.current_scene_module.post_effect then
                self:start_posteffect()
            else
                -- PostEffect無しの場合はswapしてフロントに反映
                self.data:swap()
                self:update_texture()
            end
        end
    
    -- PostEffect Multi-threaded update
    elseif #self.posteffect_workers > 0 then
        local all_done = true
        
        for _, worker in ipairs(self.posteffect_workers) do
            if not worker:is_done() then
                all_done = false
            end
        end
        
        -- PostEffect中もバックバッファからテクスチャを更新
        self:update_texture_from_back()
        
        if all_done then
            local end_time = app.get_ticks()
            print(string.format("PostEffect finished (Multi-threaded). Time: %d ms", end_time - self.render_start_time))
            self.posteffect_workers = {}
            -- PostEffect完了後にswap
            self.data:swap()
            self:update_texture()
        end
    
    -- PostEffect Single-threaded coroutine update
    elseif self.posteffect_coroutine then
        local status = coroutine.status(self.posteffect_coroutine)
        if status == "suspended" then
            local success, err = coroutine.resume(self.posteffect_coroutine)
            if not success then
                print("PostEffect Coroutine error: " .. tostring(err))
                self.posteffect_coroutine = nil
            end
            self:update_texture_from_back()
        elseif status == "dead" then
            local end_time = app.get_ticks()
            print(string.format("PostEffect finished. Time: %d ms", end_time - self.render_start_time))
            self.posteffect_coroutine = nil
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

-- PostEffectを開始
function RayTracer:start_posteffect()
    print("Starting PostEffect...")
    
    -- swap: フロントバッファ(完成画像) → バックバッファ(読み取り元)
    -- バックバッファ → フロントバッファへ書き込んで次回swap
    self.data:swap()
    
    if self.use_multithreading then
        self:start_posteffect_threads()
    else
        self.posteffect_coroutine = self:create_posteffect_coroutine()
    end
end

-- PostEffectスレッドを開始（ブロック単位分割、スレッド制限）
function RayTracer:start_posteffect_threads()
    local json = require("lib.json")
    
    self.posteffect_workers = {}
    
    -- ブロック単位で画面を分割
    local blocks = BlockUtils.generate_blocks(
        self.width, self.height, self.BLOCK_SIZE, self.NUM_THREADS
    )
    
    -- スレッドIDごとにブロックをグループ化
    local groups = BlockUtils.group_blocks_by_thread(blocks, self.NUM_THREADS)
    
    -- 各スレッドに対してワーカーを起動（NUM_THREADS個のみ）
    for thread_id = 0, self.NUM_THREADS - 1 do
        local thread_blocks = groups[thread_id]
        if #thread_blocks > 0 then
            -- ブロックリストをJSONでAppDataに保存
            local blocks_key = "posteffect_blocks_" .. thread_id
            self.data:set_string(blocks_key, json.encode(thread_blocks))
            
            -- 最初のブロックをboundsとして渡す
            local first_block = thread_blocks[1]
            local worker = ThreadWorker.create(
                self.data, self.scene,
                first_block.x, first_block.y, first_block.w, first_block.h,
                thread_id
            )
            worker:start("posteffect_worker.lua", self.current_scene_type)
            table.insert(self.posteffect_workers, worker)
        end
    end
end

-- PostEffect用コルーチン作成
function RayTracer:create_posteffect_coroutine()
    return coroutine.create(function()
        print("Starting PostEffect (Coroutine)...")
        local startTime = app.get_ticks()
        local frameAllowance = 12
        
        for y = 0, self.height - 1 do
            for x = 0, self.width - 1 do
                self.current_scene_module.post_effect(self.data, x, y)
                
                if x % 100 == 0 then
                    if (app.get_ticks() - startTime) >= frameAllowance then
                        coroutine.yield()
                        startTime = app.get_ticks()
                    end
                end
            end
        end
        
        -- PostEffect完了後にswap
        self.data:swap()
    end)
end

function RayTracer:on_ui()
    if ImGui.Begin("Lua Ray Tracer Control") then
        ImGui.Text("Welcome to Lua Ray Tracer!")
        ImGui.Text(string.format("Resolution: %d x %d", self.width, self.height))

        local is_rendering = (#self.workers > 0) or (self.render_coroutine ~= nil) or (#self.posteffect_workers > 0) or (self.posteffect_coroutine ~= nil)
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
        
        -- Resolution Presets Selection
        ImGui.Text("Resolution:")
        local presets = ResolutionPresets.get_presets()
        local current_preset = presets[self.current_preset_index]
        local preview = current_preset and current_preset.name or "Unknown"
        
        if ImGui.BeginCombo("##resolution", preview) then
            for i, preset in ipairs(presets) do
                local is_selected = (i == self.current_preset_index)
                if ImGui.Selectable(preset.name, is_selected) then
                    if i ~= self.current_preset_index then
                        self.current_preset_index = i
                        self:set_resolution(preset.width, preset.height)
                    end
                end
            end
            ImGui.EndCombo()
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
        
        if ImGui.RadioButton("Triangle", type == "triangle") then
            if type ~= "triangle" then
                self:reset_scene("triangle")
            end
        end
        
        if ImGui.RadioButton("Sphere", type == "sphere") then
            if type ~= "sphere" then
                self:reset_scene("sphere")
            end
        end
        
        if ImGui.RadioButton("MatSphere", type == "materialed_sphere") then
            if type ~= "materialed_sphere" then
                self:reset_scene("materialed_sphere")
            end
        end
        
        if ImGui.RadioButton("PostEffect", type == "posteffect") then
            if type ~= "posteffect" then
                self:reset_scene("posteffect")
            end
        end
        
        if ImGui.RadioButton("MatTransfer", type == "material_transfer") then
            if type ~= "material_transfer" then
                self:reset_scene("material_transfer")
            end
        end
        
        if ImGui.RadioButton("RTWeekend", type == "raytracing_weekend") then
            if type ~= "raytracing_weekend" then
                self:reset_scene("raytracing_weekend")
            end
        end
        
        if ImGui.RadioButton("CornellBox", type == "cornell_box") then
            if type ~= "cornell_box" then
                self:reset_scene("cornell_box")
            end
        end

        ImGui.Separator()
        
        if ImGui.Button("Reload Scene") then
            print("Reloading scene module and re-rendering")
            self:reset_scene(self.current_scene_type, true) -- force_reload = true
        end
        ImGui.EndDisabled()
        
        ImGui.Separator()

        -- Terminate Button (Enable only when rendering)
        ImGui.BeginDisabled(not is_rendering)
        if ImGui.Button("Terminate") then
            self:cancel()
        end
        ImGui.EndDisabled()

        
        ImGui.Separator()
        
        if #self.workers > 0 then
            ImGui.Text(string.format("Status: Rendering... (%d threads)", #self.workers))
        elseif self.render_coroutine then
            ImGui.Text("Status: Rendering... (Single-threaded)")
        elseif #self.posteffect_workers > 0 then
            ImGui.Text(string.format("Status: PostEffect... (%d threads)", #self.posteffect_workers))
        elseif self.posteffect_coroutine then
            ImGui.Text("Status: PostEffect... (Single-threaded)")
        else
            ImGui.Text("Status: Idle")
        end
    end
    ImGui.End()
end

return RayTracer
