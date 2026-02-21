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

-- 前方向に移動（position と look_at を同時に移動）
function Camera:move_forward(distance)
    local dx = self.forward[1] * distance
    local dy = self.forward[2] * distance
    local dz = self.forward[3] * distance
    
    self.position[1] = self.position[1] + dx
    self.position[2] = self.position[2] + dy
    self.position[3] = self.position[3] + dz
    
    self.look_at[1] = self.look_at[1] + dx
    self.look_at[2] = self.look_at[2] + dy
    self.look_at[3] = self.look_at[3] + dz
    
    self:compute_camera_basis()
end

-- 右方向に移動（position と look_at を同時に移動）
function Camera:move_right(distance)
    local dx = self.right[1] * distance
    local dy = self.right[2] * distance
    local dz = self.right[3] * distance
    
    self.position[1] = self.position[1] + dx
    self.position[2] = self.position[2] + dy
    self.position[3] = self.position[3] + dz
    
    self.look_at[1] = self.look_at[1] + dx
    self.look_at[2] = self.look_at[2] + dy
    self.look_at[3] = self.look_at[3] + dz
    
    self:compute_camera_basis()
end

-- 上方向に移動（position と look_at を同時に移動）
function Camera:move_up(distance)
    local dx = self.camera_up[1] * distance
    local dy = self.camera_up[2] * distance
    local dz = self.camera_up[3] * distance
    
    self.position[1] = self.position[1] + dx
    self.position[2] = self.position[2] + dy
    self.position[3] = self.position[3] + dz
    
    self.look_at[1] = self.look_at[1] + dx
    self.look_at[2] = self.look_at[2] + dy
    self.look_at[3] = self.look_at[3] + dz
    
    self:compute_camera_basis()
end

-- カメラの回転（yaw, pitch）
-- @param delta_yaw: Y軸周りの回転量（度数法）
-- @param delta_pitch: ローカル右軸周りの回転量（度数法）
function Camera:rotate(delta_yaw, delta_pitch)
    local rad_yaw = delta_yaw * math.pi / 180.0
    local rad_pitch = delta_pitch * math.pi / 180.0
    
    local fx, fy, fz = self.forward[1], self.forward[2], self.forward[3]
    
    -- 1. Yaw回転 (ワールドY軸: 0, 1, 0 に沿って回転)
    -- Y軸回転の標準的な回転行列: (x, y, z) -> (x*cos + z*sin, y, -x*sin + z*cos)
    local cos_y = math.cos(rad_yaw)
    local sin_y = math.sin(rad_yaw)
    
    local new_fx_y = fx * cos_y + fz * sin_y
    local new_fy_y = fy
    local new_fz_y = -fx * sin_y + fz * cos_y
    
    -- 一旦Yawのみ適用した新たなForwardベクトル
    fx, fy, fz = new_fx_y, new_fy_y, new_fz_y
    
    -- 2. Pitch回転 (ローカルRight軸に沿って回転, ロドリゲスの回転公式)
    local rx, ry, rz = self.right[1], self.right[2], self.right[3]
    local cos_p = math.cos(rad_pitch)
    local sin_p = math.sin(rad_pitch)
    
    -- 外積 k x v  (k=right, v=forward)
    local kxv_x = ry * fz - rz * fy
    local kxv_y = rz * fx - rx * fz
    local kxv_z = rx * fy - ry * fx
    
    -- 新しいForwardベクトルを計算 v_rot = v*cos + (k x v)*sin
    -- (k . v = 0 なので第3項は不要)
    local new_fx = fx * cos_p + kxv_x * sin_p
    local new_fy = fy * cos_p + kxv_y * sin_p
    local new_fz = fz * cos_p + kxv_z * sin_p
    
    -- ジンバルロック対策 (真上・真下を向きすぎないように制限)
    local len = math.sqrt(new_fx^2 + new_fy^2 + new_fz^2)
    if len > 0 then
        new_fx = new_fx / len
        new_fy = new_fy / len
        new_fz = new_fz / len
        -- Y成分が極端に近い場合（±0.99）はPitch回転をキャンセルしYawのみ適用
        if math.abs(new_fy) > 0.99 then
            new_fx, new_fy, new_fz = new_fx_y, new_fy_y, new_fz_y
            local len_y = math.sqrt(new_fx^2 + new_fy^2 + new_fz^2)
            if len_y > 0 then
                new_fx, new_fy, new_fz = new_fx / len_y, new_fy / len_y, new_fz / len_y
            end
        end
    end
    
    -- look_atを更新
    self.look_at[1] = self.position[1] + new_fx
    self.look_at[2] = self.position[2] + new_fy
    self.look_at[3] = self.position[3] + new_fz
    
    -- 基底を再計算してrightとupを更新
    self:compute_camera_basis()
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
