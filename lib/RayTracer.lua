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
    
    -- シーンを再セットアップ（reset_sceneを利用）
    self:reset_scene(self.current_scene_type)
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

-- ワーカーのみをソフトリセット（setup/cleanupは呼ばない、テクスチャクリアなし）
-- stop → start を呼び直し、レンダリングブロックを再作成してレンダリングを再開する
function RayTracer:reset_workers()
    print("Resetting workers...")
    
    -- 実行中のワーカーを安全に停止（マルチスレッド時はワーカー内でstopが呼ばれる）
    self:terminate_workers()
    
    -- シングルスレッドのコルーチンを停止
    if self.render_coroutine or self.posteffect_coroutine then
        -- シングルスレッド時はメインスレッドでstopを呼ぶ
        if self.current_scene_module and self.current_scene_module.stop then
            self.current_scene_module.stop(self.scene)
        end
    end
    self.render_coroutine = nil
    self.posteffect_coroutine = nil
    
    -- start を呼び直す（カメラやローカル状態の再初期化）
    if self.current_scene_module and self.current_scene_module.start then
        self.current_scene_module.start(self.scene, self.data)
    end
    
    -- レンダリングを再開（ブロック再作成含む、テクスチャクリアなし）
    self:render_without_clear()
end

-- テクスチャクリアなしでレンダリングを開始
function RayTracer:render_without_clear()
    print("Starting render (without clear)...")
    self.render_start_time = app.get_ticks()
    
    if self.use_multithreading then
        self:start_render_threads()
    else
        self.render_coroutine = self:create_render_coroutine()
    end
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
    if self.render_coroutine or self.posteffect_coroutine then
        if self.current_scene_module and self.current_scene_module.stop then
            self.current_scene_module.stop(self.scene)
        end
    end
    self.render_coroutine = nil
    self.posteffect_coroutine = nil
    
    -- 状態をアイドルに戻すための追加処理があれば記述
    -- 例: プログレスバーのリセットなど
end

-- レンダリング中であればキャンセルする
function RayTracer:cancel_if_rendering()
    local is_rendering = (#self.workers > 0) or (self.render_coroutine ~= nil) or (#self.posteffect_workers > 0) or (self.posteffect_coroutine ~= nil)
    if is_rendering then
        self:cancel()
    end
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
        self.width, self.height, self.BLOCK_SIZE, 1 -- スレッド数は分割には無関係なので1
    )
    
    -- ブロックをシャッフルしてランダム順序にする
    blocks = BlockUtils.shuffle_blocks(blocks)
    
    -- 共有キューをセットアップ
    BlockUtils.setup_shared_queue(self.data, blocks, "render_queue")

    -- ワーカーを作成して開始
    for i = 0, self.NUM_THREADS - 1 do
        -- Boundsは使用しないが、一応画面全体を渡しておく
        local worker = ThreadWorker.create(self.data, self.scene, 0, 0, self.width, self.height, i)
        
        -- ワーカー開始 (ブロック情報は共有キューから取得するため個別設定不要)
        worker:start("workers/ray_worker.lua", self.current_scene_type)
        table.insert(self.workers, worker)
    end
end

-- シングルスレッドレンダリング用コルーチン作成
function RayTracer:create_render_coroutine()
    return coroutine.create(function()
        print("Starting single-threaded render (Coroutine)...")
        local startTime = app.get_ticks()
        local frameAllowance = 12 -- 12ms
        
        -- 移動平均初期化
        local time_avg = BlockUtils.MovingAverage.new(0.1)
        local estimated_time = 0
        
        for y = 0, self.height - 1 do
            for x = 0, self.width - 1 do
                -- 推定時間が閾値を超えたらチェック
                if estimated_time >= frameAllowance then
                    if (app.get_ticks() - startTime) >= frameAllowance then
                        coroutine.yield()
                        startTime = app.get_ticks() -- Reset start time for next slice
                    end
                    estimated_time = 0
                end

                local pixel_start = app.get_ticks()
                self.current_scene_module.shade(self.data, x, y)
                local pixel_end = app.get_ticks()
                
                -- 処理時間を計測して移動平均を更新
                local elapsed = pixel_end - pixel_start
                if elapsed <= 0 then
                    elapsed = 0.001
                end
                time_avg:update(elapsed)
                
                -- 推定時間を更新
                estimated_time = estimated_time + time_avg:get()
            end
        end
        
        print(string.format("Single-threaded render finished internally."))
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
                
                -- シーン終了
                if self.current_scene_module and self.current_scene_module.stop then
                    self.current_scene_module.stop(self.scene)
                end
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
            
            -- シーン終了
            if self.current_scene_module and self.current_scene_module.stop then
                self.current_scene_module.stop(self.scene)
            end
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
        self.width, self.height, self.BLOCK_SIZE, 1 -- スレッド数無関係
    )
    
    -- ブロックをシャッフルしてランダム順序にする
    blocks = BlockUtils.shuffle_blocks(blocks)
    
    -- 共有キューをセットアップ
    BlockUtils.setup_shared_queue(self.data, blocks, "posteffect_queue")
    
    -- 各スレッドに対してワーカーを起動
    for i = 0, self.NUM_THREADS - 1 do
        -- Boundsは使用しないが画面全体を渡す
        local worker = ThreadWorker.create(
            self.data, self.scene, 0, 0, self.width, self.height, i
        )
        worker:start("workers/posteffect_worker.lua", self.current_scene_type)
        table.insert(self.posteffect_workers, worker)
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

        -- Render Mode Selection
        ImGui.Text("Render Mode:")
        if ImGui.RadioButton("Single-threaded", not self.use_multithreading) then
            if self.use_multithreading then
                self:cancel_if_rendering()
                self.use_multithreading = false
                self:render()
            end
        end
        ImGui.SameLine()
        if ImGui.RadioButton("Multi-threaded", self.use_multithreading) then
            if not self.use_multithreading then
                self:cancel_if_rendering()
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
                        self:cancel_if_rendering()
                        self.current_preset_index = i
                        self:set_resolution(preset.width, preset.height)
                    end
                end
            end
            ImGui.EndCombo()
        end

        ImGui.Separator()
        
        ImGui.Text("Scene Selection:")
        
        -- Define scenes list
        local scenes = {
            { id = "color_pattern", name = "Color Pattern" },
            { id = "triangle", name = "Triangle" },
            { id = "sphere", name = "Sphere" },
            { id = "materialed_sphere", name = "MatSphere" },
            { id = "wasd_camera", name = "WASD Camera" },
            { id = "posteffect", name = "PostEffect" },
            { id = "material_transfer", name = "MatTransfer" },
            { id = "gltf_box", name = "GLTF Box" },
            { id = "test_lifecycle", name = "Test Lifecycle" },
            { id = "raytracing_weekend", name = "RTWeekend" },
            { id = "cornell_box", name = "CornellBox" },
        }
        
        local current_scene_name = "Unknown"
        for _, scene in ipairs(scenes) do
            if scene.id == self.current_scene_type then
                current_scene_name = scene.name
                break
            end
        end
        
        if ImGui.BeginCombo("##scene_selection", current_scene_name) then
            for _, scene in ipairs(scenes) do
                local is_selected = (scene.id == self.current_scene_type)
                if ImGui.Selectable(scene.name, is_selected) then
                    if self.current_scene_type ~= scene.id then
                        self:cancel_if_rendering()
                        self:reset_scene(scene.id)
                    end
                end
            end
            ImGui.EndCombo()
        end

        ImGui.Separator()
        
        if ImGui.Button("Reload Scene") then
            self:cancel_if_rendering()
            print("Reloading scene module and re-rendering")
            self:reset_scene(self.current_scene_type, true) -- force_reload = true
        end
        
        ImGui.SameLine()
        
        if ImGui.Button("Reset Workers") then
            self:cancel_if_rendering()
            self:reset_workers()
        end
        
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

-- キーボード入力を処理してカメラを移動させる
function RayTracer:handle_keyboard()
    if not app.get_keyboard_state then return end
    
    local state = app.get_keyboard_state()
    if not state then return end
    
    -- シーンにカメラが存在するか確認
    if not self.current_scene_module or not self.current_scene_module.get_camera then
        return
    end
    
    local camera = self.current_scene_module:get_camera()
    if not camera then return end
    
    local moved = false
    local speed = 0.5 -- 1フレームあたりの移動速度
    
    -- キーボードの状態に応じて移動
    if state["w"] or state["up"] then
        camera:move_forward(speed)
        moved = true
    end
    if state["s"] or state["down"] then
        camera:move_forward(-speed)
        moved = true
    end
    if state["d"] or state["right"] then
        camera:move_right(speed)
        moved = true
    end
    if state["a"] or state["left"] then
        camera:move_right(-speed)
        moved = true
    end
    if state["e"] or state["space"] then
        camera:move_up(speed)
        moved = true
    end
    if state["q"] or state["c"] then
        camera:move_up(-speed)
        moved = true
    end
    
    -- カメラが移動した場合、レイトレーシングをリセットして再描画
    if moved then
        self:reset_workers()
    end
end

return RayTracer
