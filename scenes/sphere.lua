-- scenes/sphere.lua
-- 球体シーンを定義するモジュール

local M = {}

-- カメラの作成: 透視投影
function M.create_camera(aspect_ratio)
    local Camera = require("camera")
    return Camera.new("perspective", {
        position = {0, 0, 2},
        look_at = {0, 0, 0},
        up = {0, 1, 0},
        aspect_ratio = aspect_ratio,
        fov = 60.0
    })
end

-- シーンのセットアップ: 球体を追加
function M.setup(scene)
    print("Creating Sphere Scene...")
    -- 原点に半径0.5の球体を配置
    scene:add_sphere(0.0, 0.0, 0.0, 0.5)
end

-- ピクセルの色を計算(ディフューズシェーディング)
function M.shade(hit, x, flip_y, data, u, v, ox, oy, oz, dx, dy, dz, nx, ny, nz, lightDirX, lightDirY, lightDirZ)
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
