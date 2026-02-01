-- scenes/color_pattern.lua
-- カラーパターンシーンを定義するモジュール

local M = {}

-- カメラの作成: 並行投影
function M.create_camera(aspect_ratio)
    local Camera = require("camera")
    return Camera.new("orthographic", {
        position = {0, 0, 1},
        look_at = {0, 0, 0},
        up = {0, 1, 0},
        aspect_ratio = aspect_ratio,
        ortho_height = 2.0
    })
end

-- シーンのセットアップ: オブジェクトなし(背景パターンのみ)
function M.setup(scene)
    print("Creating Color Pattern Scene...")
    -- オブジェクトなし、背景パターンのみ
end

-- ピクセルの色を計算(法線/スクリーン座標に基づくカラーリング)
function M.shade(hit, x, flip_y, data, u, v, ox, oy, oz, dx, dy, dz, nx, ny, nz, lightDirX, lightDirY, lightDirZ)
    if hit then
        -- 法線に基づくカラーリング
        -- 法線 [-1, 1] を [0, 1] -> [0, 255] にマッピング
        local r = math.floor((nx + 1.0) * 0.5 * 255)
        local g = math.floor((ny + 1.0) * 0.5 * 255)
        local b = math.floor((nz + 1.0) * 0.5 * 255)
        data:set_pixel(x, flip_y, r, g, b)
    else
        -- スクリーン座標に基づく背景(並行投影でも機能する)
        -- u, v はすでに [-1, 1] 範囲に正規化されている
        local r = math.floor((u + 1.0) * 0.5 * 255)
        local g = math.floor((v + 1.0) * 0.5 * 255)
        local b = 128  -- 固定値
        data:set_pixel(x, flip_y, r, g, b)
    end
end

return M
