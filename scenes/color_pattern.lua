-- scenes/color_pattern.lua
-- カラーパターンシーンを定義するモジュール

local M = {}

-- シーンのセットアップ: オブジェクトなし（背景パターンのみ）
function M.setup(scene)
    print("Creating Color Pattern Scene...")
    -- オブジェクトなし、背景パターンのみ
end

-- ピクセルの色を計算（法線/レイ方向に基づくカラーリング）
function M.shade(hit, x, flip_y, data, dx, dy, dz, nx, ny, nz, lightDirX, lightDirY, lightDirZ)
    if hit then
        -- 法線に基づくカラーリング
        -- 法線 [-1, 1] を [0, 1] -> [0, 255] にマッピング
        local r = math.floor((nx + 1.0) * 0.5 * 255)
        local g = math.floor((ny + 1.0) * 0.5 * 255)
        local b = math.floor((nz + 1.0) * 0.5 * 255)
        data:set_pixel(x, flip_y, r, g, b)
    else
        -- レイ方向に基づく背景
        local r = math.floor((dx + 1.0) * 0.5 * 255)
        local g = math.floor((dy + 1.0) * 0.5 * 255)
        local b = math.floor((dz + 1.0) * 0.5 * 255)
        data:set_pixel(x, flip_y, r, g, b)
    end
end

return M
