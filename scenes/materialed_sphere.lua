-- scenes/materialed_sphere.lua
-- マテリアル付き球体シーンを定義するモジュール

local M = {}

-- モジュール内でシーン、カメラ、サイズを保持
local scene = nil
local camera = nil
local width = 0
local height = 0

-- マテリアルテーブル (geomID -> Material)
local materials = {}

-- ライト方向（正規化済み）
local lightDirX, lightDirY, lightDirZ = 0.577, 0.577, 0.577 -- 斜め上くから
local len = math.sqrt(lightDirX*lightDirX + lightDirY*lightDirY + lightDirZ*lightDirZ)
lightDirX, lightDirY, lightDirZ = lightDirX/len, lightDirY/len, lightDirZ/len

-- シーンのセットアップ: 球体を追加
function M.setup(embree_scene, app_data)
    print("Setup Materialed Sphere Scene...")
    
    -- 球体1: 赤い球体 (ID 0)
    local id1 = embree_scene:add_sphere(-1.5, 0.0, 0.0, 1.0)
    
    -- 球体2: 緑の球体 (ID 1)
    local id2 = embree_scene:add_sphere(1.5, 0.0, 0.0, 1.0)
    
    -- 球体3: 青い球体 (小さい) (ID 2)
    local id3 = embree_scene:add_sphere(0.0, 1.0, -1.0, 0.5)

    -- 球体4: 黄色の球体 (手前) (ID 3)
    local id4 = embree_scene:add_sphere(0.0, -0.5, 1.5, 0.3)
    
    print("Spheres added. IDs:", id1, id2, id3, id4)
end

-- シーンの開始: カメラとローカル変数の初期化、マテリアルの初期化
function M.start(embree_scene, app_data)
    print("Start Materialed Sphere Scene...")
    scene = embree_scene
    width = app_data:width()
    height = app_data:height()
    local aspect_ratio = width / height
    
    -- マテリアルテーブルを初期化 (setupでの追加順序に依存)
    materials = {}
    materials[0] = {r = 1.0, g = 0.2, b = 0.2} -- 赤
    materials[1] = {r = 0.2, g = 1.0, b = 0.2} -- 緑
    materials[2] = {r = 0.2, g = 0.2, b = 1.0} -- 青
    materials[3] = {r = 1.0, g = 1.0, b = 0.0} -- 黄
    
    -- カメラの作成: 透視投影
    local CameraUtils = require("lib.CameraUtils")
    camera = CameraUtils.setup_or_sync_camera(camera, app_data, {
        position = {0, 0, 5}, -- 少し離れた位置から
        look_at = {0, 0, 0},
        up = {0, 1, 0},
        aspect_ratio = aspect_ratio,
        fov = 60.0
    })
end

-- ピクセルの色を計算
function M.shade(data, x, y)
    -- 整数座標から正規化座標 [-1, 1] を計算
    local u = (2.0 * x - width) / width
    local v = (2.0 * y - height) / height
    
    -- カメラからレイを生成
    local ox, oy, oz, dx, dy, dz = camera:generate_ray(u, v)
    
    -- シーンとのインターセクト判定
    -- 戻り値: hit, t, nx, ny, nz, geomID, primID
    local hit, t, nx, ny, nz, geomID, primID = scene:intersect(ox, oy, oz, dx, dy, dz)
    
    -- Y座標を上下反転
    local flip_y = height - 1 - y
    
    if hit then
        -- マテリアル取得
        local material = materials[geomID]
        local r, g, b = 1.0, 1.0, 1.0
        
        if material then
            r, g, b = material.r, material.g, material.b
        else
            -- マテリアルが見つからない場合はマゼンタ (デバッグ用)
            r, g, b = 1.0, 0.0, 1.0
        end
        
        -- ディフューズシェーディング計算
        local diffuse = nx * lightDirX + ny * lightDirY + nz * lightDirZ
        if diffuse < 0 then diffuse = 0 end
        
        -- アンビエント光 + ディフューズ
        local intensity = 0.2 + 0.8 * diffuse
        
        -- 色を適用
        r = r * intensity
        g = g * intensity
        b = b * intensity
        
        -- クランプ
        if r > 1.0 then r = 1.0 end
        if g > 1.0 then g = 1.0 end
        if b > 1.0 then b = 1.0 end
        
        data:set_pixel(x, flip_y, math.floor(255 * r), math.floor(255 * g), math.floor(255 * b))
    else
        -- 
        -- 空のようなグラデーション
        local t_bg = 0.5 * (dy + 1.0) -- dir.y は -1 ~ 1 なので 0 ~ 1 に
        local r = (1.0 - t_bg) * 1.0 + t_bg * 0.5
        local g = (1.0 - t_bg) * 1.0 + t_bg * 0.7
        local b = (1.0 - t_bg) * 1.0 + t_bg * 1.0
        
        data:set_pixel(x, flip_y, math.floor(255 * r), math.floor(255 * g), math.floor(255 * b))
    end
end

-- 外部からカメラインスタンスを取得できるようにする
function M.get_camera()
    return camera
end

-- クリーンアップ処理
function M.cleanup()
    camera = nil -- シーンリセット時にカメラも完全に初期化させる
end

return M
