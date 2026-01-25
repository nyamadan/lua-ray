-- scenes/triangle.lua
-- 三角形シーンを定義するモジュール

local M = {}

-- シーンのセットアップ: 三角形を追加
function M.setup(scene)
    print("Creating Triangle Scene...")
    -- 三角形の頂点: (-0.5, -0.5, 0), (0.5, -0.5, 0), (0.0, 0.5, 0)
    scene:add_triangle(
        -0.5, -0.5, 0.0,
         0.5, -0.5, 0.0,
         0.0,  0.5, 0.0
    )
end

-- ピクセルの色を計算（黄色がかったディフューズシェーディング）
function M.shade(hit, x, flip_y, data, dx, dy, dz, nx, ny, nz, lightDirX, lightDirY, lightDirZ)
    if hit then
        -- ディフューズシェーディング計算
        local diffuse = nx * lightDirX + ny * lightDirY + nz * lightDirZ
        if diffuse < 0 then diffuse = 0 end
        
        -- アンビエント光を追加
        diffuse = 0.2 + 0.8 * diffuse
        if diffuse > 1.0 then diffuse = 1.0 end
        
        local shade = math.floor(255 * diffuse)
        -- 黄色がかったティント
        data:set_pixel(x, flip_y, math.floor(shade * 1.0), math.floor(shade * 0.8), math.floor(shade * 0.8))
    else
        -- 暗い青っぽい背景
        data:set_pixel(x, flip_y, 50, 50, 60)
    end
end

return M
