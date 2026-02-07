-- scenes/posteffect.lua
-- ポストエフェクト（グレースケール）付きシーン

local M = {}

-- モジュール内でシーン、カメラ、サイズを保持
local scene = nil
local camera = nil
local width = 0
local height = 0

-- マテリアルテーブル (geomID -> Material)
local materials = {}

-- ライト方向（正規化済み）
local lightDirX, lightDirY, lightDirZ = 0.577, 0.577, 0.577

-- シーンのセットアップ: 球体を追加
function M.setup(embree_scene, app_data)
    print("Setup PostEffect Scene...")
    
    -- 球体1: 赤い球体 (ID 0)
    local id1 = embree_scene:add_sphere(-1.5, 0.0, 0.0, 1.0)
    
    -- 球体2: 緑の球体 (ID 1)
    local id2 = embree_scene:add_sphere(1.5, 0.0, 0.0, 1.0)
    
    -- 球体3: 青い球体 (小さい) (ID 2)
    local id3 = embree_scene:add_sphere(0.0, 1.0, -1.0, 0.5)
    
    print("Spheres added. IDs:", id1, id2, id3)
end

-- シーンの開始: カメラとローカル変数の初期化
function M.start(embree_scene, app_data)
    print("Start PostEffect Scene...")
    scene = embree_scene
    width = app_data:width()
    height = app_data:height()
    local aspect_ratio = width / height
    
    -- マテリアルテーブルを初期化
    materials = {}
    materials[0] = {r = 1.0, g = 0.2, b = 0.2} -- 赤
    materials[1] = {r = 0.2, g = 1.0, b = 0.2} -- 緑
    materials[2] = {r = 0.2, g = 0.2, b = 1.0} -- 青
    
    -- カメラの作成: 透視投影
    local Camera = require("lib.Camera")
    camera = Camera.new("perspective", {
        position = {0, 0, 5},
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
            r, g, b = 1.0, 0.0, 1.0
        end
        
        -- ディフューズシェーディング計算
        local diffuse = nx * lightDirX + ny * lightDirY + nz * lightDirZ
        if diffuse < 0 then diffuse = 0 end
        
        -- アンビエント光 + ディフューズ
        local intensity = 0.2 + 0.8 * diffuse
        
        r = r * intensity
        g = g * intensity
        b = b * intensity
        
        -- クランプ
        if r > 1.0 then r = 1.0 end
        if g > 1.0 then g = 1.0 end
        if b > 1.0 then b = 1.0 end
        
        data:set_pixel(x, flip_y, math.floor(255 * r), math.floor(255 * g), math.floor(255 * b))
    else
        -- 空のようなグラデーション
        local t_bg = 0.5 * (dy + 1.0)
        local r = (1.0 - t_bg) * 1.0 + t_bg * 0.5
        local g = (1.0 - t_bg) * 1.0 + t_bg * 0.7
        local b = (1.0 - t_bg) * 1.0 + t_bg * 1.0
        
        data:set_pixel(x, flip_y, math.floor(255 * r), math.floor(255 * g), math.floor(255 * b))
    end
end

-- ポストエフェクト: グレースケール変換
-- フロントバッファから読み取り、バックバッファに書き込む
function M.post_effect(data, x, y)
    local r, g, b = data:get_pixel(x, y)
    
    -- グレースケール変換（Rec.601）
    local gray = math.floor(0.299 * r + 0.587 * g + 0.114 * b)
    
    data:set_pixel(x, y, gray, gray, gray)
end

return M
