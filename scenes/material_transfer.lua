-- scenes/material_transfer.lua
-- setupとstartの間でマテリアルを受け渡すテストシーン
-- app_dataのset_string/get_stringとlib.jsonでJSONシリアライズを使用

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
local len = math.sqrt(lightDirX*lightDirX + lightDirY*lightDirY + lightDirZ*lightDirZ)
lightDirX, lightDirY, lightDirZ = lightDirX/len, lightDirY/len, lightDirZ/len

-- シーンのセットアップ: ジオメトリ作成とマテリアルの保存
function M.setup(embree_scene, app_data)
    print("Setup Material Transfer Scene...")
    
    -- 球体を追加
    local id1 = embree_scene:add_sphere(-1.5, 0.0, 0.0, 1.0)
    local id2 = embree_scene:add_sphere(1.5, 0.0, 0.0, 1.0)
    local id3 = embree_scene:add_sphere(0.0, 1.0, -1.0, 0.5)
    local id4 = embree_scene:add_sphere(0.0, -0.5, 1.5, 0.3)
    
    print("Spheres added. IDs:", id1, id2, id3, id4)
    
    -- マテリアルテーブルを作成
    local material_data = {
        [tostring(id1)] = {r = 1.0, g = 0.2, b = 0.2}, -- 赤
        [tostring(id2)] = {r = 0.2, g = 1.0, b = 0.2}, -- 緑
        [tostring(id3)] = {r = 0.2, g = 0.2, b = 1.0}, -- 青
        [tostring(id4)] = {r = 1.0, g = 1.0, b = 0.0}, -- 黄
    }
    
    -- JSONにシリアライズしてapp_dataに保存
    local json = require("lib.json")
    local json_str = json.encode(material_data)
    app_data:set_string("materials", json_str)
    
    print("Materials serialized and stored:", json_str)
end

-- シーンの開始: マテリアルをデシリアライズして使用
function M.start(embree_scene, app_data)
    print("Start Material Transfer Scene...")
    scene = embree_scene
    width = app_data:width()
    height = app_data:height()
    local aspect_ratio = width / height
    
    -- app_dataからマテリアルを取得してデシリアライズ
    local json = require("lib.json")
    local json_str = app_data:get_string("materials")
    
    if json_str and json_str ~= "" then
        local material_data = json.decode(json_str)
        -- 数値キーに変換
        for k, v in pairs(material_data) do
            materials[tonumber(k)] = v
        end
        print("Materials deserialized:", json_str)
    else
        print("Warning: No materials found in app_data!")
        -- フォールバック: デフォルトマテリアル
        materials[0] = {r = 1.0, g = 0.0, b = 1.0} -- マゼンタ（エラー表示用）
    end
    
    -- カメラの作成: 透視投影
    local CameraUtils = require("lib.CameraUtils")
    camera = CameraUtils.setup_or_sync_camera(camera, app_data, {
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
        -- 空のようなグラデーション
        local t_bg = 0.5 * (dy + 1.0)
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
