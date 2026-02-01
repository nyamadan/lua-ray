-- scenes/color_pattern.lua
-- カラーパターンシーンを定義するモジュール

local M = {}

-- モジュール内でカメラとサイズを保持
local camera = nil
local width = 0
local height = 0

-- シーンのセットアップ: オブジェクトなし(背景パターンのみ)
function M.setup(embree_scene, app_data)
    print("Creating Color Pattern Scene...")
    width = app_data:width()
    height = app_data:height()
    local aspect_ratio = width / height
    -- オブジェクトなし、背景パターンのみ
    
    -- カメラの作成: 並行投影
    local Camera = require("camera")
    camera = Camera.new("orthographic", {
        position = {0, 0, 1},
        look_at = {0, 0, 0},
        up = {0, 1, 0},
        aspect_ratio = aspect_ratio,
        ortho_height = 2.0
    })
    print("Camera type: " .. camera.camera_type)
end

-- ピクセルの色を計算(スクリーン座標に基づくカラーリング)
function M.shade(data, x, y)
    -- 整数座標から正規化座標 [-1, 1] を計算
    local u = (2.0 * x - width) / width
    local v = (2.0 * y - height) / height
    
    -- Y座標を上下反転(画像座標系からテクスチャ座標系への変換)
    local flip_y = height - 1 - y
    
    -- スクリーン座標に基づく背景(並行投影でも機能する)
    local r = math.floor((u + 1.0) * 0.5 * 255)
    local g = math.floor((v + 1.0) * 0.5 * 255)
    local b = 128  -- 固定値
    data:set_pixel(x, flip_y, r, g, b)
end

return M
