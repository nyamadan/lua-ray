-- camera.lua
-- カメラクラス: 透視投影と並行投影をサポート

Camera = {}
Camera.__index = Camera

-- カメラの作成
-- @param camera_type: "perspective" または "orthographic"
-- @param params: カメラパラメータのテーブル
--   - position: {x, y, z} カメラの位置
--   - look_at: {x, y, z} 注視点
--   - up: {x, y, z} 上方向ベクトル
--   - aspect_ratio: アスペクト比 (width / height)
--   - fov: 視野角(度数法) - 透視投影の場合のみ
--   - ortho_height: 投影サイズ(垂直方向) - 並行投影の場合のみ
function Camera.new(camera_type, params)
    local self = setmetatable({}, Camera)
    
    self.camera_type = camera_type or "perspective"
    
    -- カメラパラメータ
    self.position = params.position or {0, 0, 1}
    self.look_at = params.look_at or {0, 0, 0}
    self.up = params.up or {0, 1, 0}
    self.aspect_ratio = params.aspect_ratio or 1.0
    
    -- 透視投影パラメータ
    self.fov = params.fov or 60.0  -- 度数法
    
    -- 並行投影パラメータ
    self.ortho_height = params.ortho_height or 2.0
    
    -- 最適化: タイプに応じてレイ生成関数を直接割り当て（条件分岐を削減）
    if self.camera_type == "perspective" then
        self.generate_ray = Camera.generate_perspective_ray
    elseif self.camera_type == "orthographic" then
        self.generate_ray = Camera.generate_orthographic_ray
    else
        error("Unknown camera type: " .. tostring(self.camera_type))
    end
    
    -- カメラ座標系を計算
    self:compute_camera_basis()
    
    return self
end

-- レイを生成 (フォールバック、またはオーバーライドされる前の定義)
function Camera:generate_ray(u, v)
    -- この関数はインスタンス生成時にオーバーライドされるため、
    -- 通常は呼ばれませんが、メタテーブル経由で呼ばれた場合のフォールバックとして残します。
    if self.camera_type == "perspective" then
        return self:generate_perspective_ray(u, v)
    elseif self.camera_type == "orthographic" then
        return self:generate_orthographic_ray(u, v)
    else
        error("Unknown camera type: " .. self.camera_type)
    end
end


-- カメラ座標系の基底ベクトルを計算
function Camera:compute_camera_basis()
    -- 前方向ベクトル (look_at - position)
    local fx = self.look_at[1] - self.position[1]
    local fy = self.look_at[2] - self.position[2]
    local fz = self.look_at[3] - self.position[3]
    
    -- 正規化
    local flen = math.sqrt(fx*fx + fy*fy + fz*fz)
    if flen > 0 then
        fx, fy, fz = fx/flen, fy/flen, fz/flen
    end
    
    -- 右方向ベクトル (forward × up)
    local rx = fy * self.up[3] - fz * self.up[2]
    local ry = fz * self.up[1] - fx * self.up[3]
    local rz = fx * self.up[2] - fy * self.up[1]
    
    -- 正規化
    local rlen = math.sqrt(rx*rx + ry*ry + rz*rz)
    if rlen > 0 then
        rx, ry, rz = rx/rlen, ry/rlen, rz/rlen
    end
    
    -- 上方向ベクトル (right × forward)
    local ux = ry * fz - rz * fy
    local uy = rz * fx - rx * fz
    local uz = rx * fy - ry * fx
    
    -- 正規化
    local ulen = math.sqrt(ux*ux + uy*uy + uz*uz)
    if ulen > 0 then
        ux, uy, uz = ux/ulen, uy/ulen, uz/ulen
    end
    
    self.forward = {fx, fy, fz}
    self.right = {rx, ry, rz}
    self.camera_up = {ux, uy, uz}
end

-- 透視投影レイの生成
function Camera:generate_perspective_ray(u, v)
    -- 視野角からスクリーンサイズを計算
    local fov_rad = self.fov * math.pi / 180.0
    local half_height = math.tan(fov_rad / 2.0)
    local half_width = self.aspect_ratio * half_height
    
    -- スクリーン上の点を計算
    local screen_x = u * half_width
    local screen_y = v * half_height
    
    -- レイの方向 = forward + screen_x * right + screen_y * up
    local dx = self.forward[1] + screen_x * self.right[1] + screen_y * self.camera_up[1]
    local dy = self.forward[2] + screen_x * self.right[2] + screen_y * self.camera_up[2]
    local dz = self.forward[3] + screen_x * self.right[3] + screen_y * self.camera_up[3]
    
    -- 正規化
    local dlen = math.sqrt(dx*dx + dy*dy + dz*dz)
    if dlen > 0 then
        dx, dy, dz = dx/dlen, dy/dlen, dz/dlen
    end
    
    -- レイの原点はカメラ位置
    return self.position[1], self.position[2], self.position[3], dx, dy, dz
end

-- 並行投影レイの生成
function Camera:generate_orthographic_ray(u, v)
    -- スクリーンサイズ
    local half_height = self.ortho_height / 2.0
    local half_width = self.aspect_ratio * half_height
    
    -- スクリーン上の点を計算
    local screen_x = u * half_width
    local screen_y = v * half_height
    
    -- レイの原点 = カメラ位置 + screen_x * right + screen_y * up
    local ox = self.position[1] + screen_x * self.right[1] + screen_y * self.camera_up[1]
    local oy = self.position[2] + screen_x * self.right[2] + screen_y * self.camera_up[2]
    local oz = self.position[3] + screen_x * self.right[3] + screen_y * self.camera_up[3]
    
    -- レイの方向は前方向(すべてのレイが平行)
    local dx = self.forward[1]
    local dy = self.forward[2]
    local dz = self.forward[3]
    
    return ox, oy, oz, dx, dy, dz
end

return Camera
