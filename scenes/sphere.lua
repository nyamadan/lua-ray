-- scenes/sphere.lua
-- 球体シーンを定義するモジュール

local M = {}

-- モジュール内でシーン、カメラ、サイズを保持
local scene = nil
local camera = nil
local width = 0
local height = 0

-- ライト方向（正規化済み）
local lightDirX, lightDirY, lightDirZ = 0.707, 0.0, 0.707
local len = math.sqrt(lightDirX*lightDirX + lightDirY*lightDirY + lightDirZ*lightDirZ)
lightDirX, lightDirY, lightDirZ = lightDirX/len, lightDirY/len, lightDirZ/len

-- シーンのセットアップ: 球体を追加
function M.setup(embree_scene, app_data)
    print("Setup Sphere Scene (Geometry)...")
    -- 原点に半径0.5の球体を配置
    embree_scene:add_sphere(0.0, 0.0, 0.0, 0.5)
end

-- シーンの開始: カメラとローカル変数の初期化
function M.start(embree_scene, app_data)
    print("Start Sphere Scene (Camera & Locals)...")
    scene = embree_scene
    width = app_data:width()
    height = app_data:height()
    local aspect_ratio = width / height
    
    -- カメラの作成: 透視投影
    local Camera = require("lib.Camera")
    camera = Camera.new("perspective", {
        position = {0, 0, 2},
        look_at = {0, 0, 0},
        up = {0, 1, 0},
        aspect_ratio = aspect_ratio,
        fov = 60.0
    })
    print("Camera type: " .. camera.camera_type)
end

-- ピクセルの色を計算(ディフューズシェーディング)
function M.shade(data, x, y)
    -- 整数座標から正規化座標 [-1, 1] を計算
    local u = (2.0 * x - width) / width
    local v = (2.0 * y - height) / height
    
    -- カメラからレイを生成
    local ox, oy, oz, dx, dy, dz = camera:generate_ray(u, v)
    
    -- シーンとのインターセクト判定
    local hit, t, nx, ny, nz = scene:intersect(ox, oy, oz, dx, dy, dz)
    
    -- Y座標を上下反転(画像座標系からテクスチャ座標系への変換)
    local flip_y = height - 1 - y
    
    if hit then
        -- ディフューズシェーディング計算
        local diffuse = nx * lightDirX + ny * lightDirY + nz * lightDirZ
        if diffuse < 0 then diffuse = 0 end
        
        -- アンビエント光を追加
        diffuse = 0.2 + 0.8 * diffuse
        if diffuse > 1.0 then diffuse = 1.0 end
        
        local shade = math.floor(255 * diffuse)
        -- 白/グレーの球体
        data:set_pixel(x, flip_y, shade, shade, shade)
    else
        -- グレーの背景
        data:set_pixel(x, flip_y, 128, 128, 128)
    end
end

return M
