-- scenes/wasd_camera.lua
-- WASDカメラ操作をテストするためのシンプルなシーン

local M = {}

local scene = nil
local camera = nil
local width = 0
local height = 0

-- ライト方向（正規化済み）
local lightDirX, lightDirY, lightDirZ = 0.577, 0.577, 0.577

function M.setup(embree_scene, app_data)
    print("Setup WASD Camera Scene (Geometry)...")
    
    -- 中心に大きな球体を配置
    embree_scene:add_sphere(0.0, 0.0, 0.0, 1.0)
    
    -- 左に小さな球体を配置
    embree_scene:add_sphere(-2.0, 0.0, -1.0, 0.5)
    
    -- 右に小さな球体を配置
    embree_scene:add_sphere(2.0, 0.0, -1.0, 0.5)
    
    -- 地面（大きな球体で表現）
    embree_scene:add_sphere(0.0, -101.0, 0.0, 100.0)
end

function M.start(embree_scene, app_data)
    print("Start WASD Camera Scene (Camera & Locals)...")
    scene = embree_scene
    width = app_data:width()
    height = app_data:height()
    local aspect_ratio = width / height
    
    -- カメラが未初期化の場合のみ新しく作成（シーン切り替えや初回起動時）
    -- ワーカーのリセット(Soft reset)時は現在のカメラ状態を維持する
    if not camera then
        local Camera = require("lib.Camera")
        camera = Camera.new("perspective", {
            position = {0, 1.0, 5.0}, -- 少し上から見下ろす
            look_at = {0, 0, 0},
            up = {0, 1, 0},
            aspect_ratio = aspect_ratio,
            fov = 60.0
        })
    end
    -- ワーカー用にシリアライズされたカメラ状態が渡されていれば適用する
    -- メインスレッドではカメラ状態を直接変更しているため、JSONからの復元はワーカーのみで行う
    if _thread_id ~= nil then
        local camera_state_json = app_data:get_string("camera_state")
        if camera_state_json and camera_state_json ~= "" then
            local json = require("lib.json")
            local success, state = pcall(json.decode, camera_state_json)
            if success and state then
                -- カメラのプロパティを上書き
                if state.position then camera.position = state.position end
                if state.look_at then camera.look_at = state.look_at end
                if state.up then camera.camera_up = state.up end
                if state.aspect_ratio then camera.aspect_ratio = state.aspect_ratio end
                if state.fov then camera.fov = state.fov end
                if state.type then camera.type = state.type end
                
                -- 基底ベクトルを再計算
                camera:compute_camera_basis()
            end
        end
    end
end

-- 外部（RayTracer）からカメラインスタンスを取得できるようにする
function M.get_camera()
    return camera
end

function M.shade(data, x, y)
    local u = (2.0 * x - width) / width
    local v = (2.0 * y - height) / height
    
    -- カメラからレイを生成
    local ox, oy, oz, dx, dy, dz = camera:generate_ray(u, v)
    
    -- シーンとのインターセクト判定
    local hit, t, nx, ny, nz = scene:intersect(ox, oy, oz, dx, dy, dz)
    
    -- Y座標を上下反転
    local flip_y = height - 1 - y
    
    if hit then
        -- シンプルなランバート・ディフューズ
        local diffuse = nx * lightDirX + ny * lightDirY + nz * lightDirZ
        if diffuse < 0 then diffuse = 0 end
        diffuse = 0.2 + 0.8 * diffuse -- アンビエント
        
        -- 高さに応じて色を変える（地面と球体を見分けるため）
        local px = ox + t * dx
        local py = oy + t * dy
        local pz = oz + t * dz
        
        local r, g, b = 200, 200, 200
        
        if py < -0.5 then
            -- 地面はチェッカーボード風
            local check = math.floor(px) + math.floor(pz)
            if check % 2 == 0 then
                r, g, b = 150, 150, 150
            else
                r, g, b = 100, 100, 100
            end
        else
            -- 球体にはグラデーションをつける
            r = math.min(255, math.max(0, math.floor((nx + 1.0) * 127)))
            g = math.min(255, math.max(0, math.floor((ny + 1.0) * 127)))
            b = math.min(255, math.max(0, math.floor((nz + 1.0) * 127)))
        end
        
        r = math.floor(r * diffuse)
        g = math.floor(g * diffuse)
        b = math.floor(b * diffuse)
        
        data:set_pixel(x, flip_y, r, g, b)
    else
        -- 空の色（グラデーション）
        local t_bg = 0.5 * (dy + 1.0)
        local r = math.floor(255 * (1.0 - t_bg) + 128 * t_bg)
        local g = math.floor(255 * (1.0 - t_bg) + 178 * t_bg)
        local b = math.floor(255 * (1.0 - t_bg) + 255 * t_bg)
        
        data:set_pixel(x, flip_y, r, g, b)
    end
end

-- クリーンアップ処理
function M.cleanup()
    camera = nil -- シーンリセット時にカメラも完全に初期化させる
end

return M
