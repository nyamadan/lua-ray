-- lib/Ray.lua
-- レイクラス (Ray Tracing in One Weekend用)

local Vec3 = require('lib.Vec3')

local Ray = {}
Ray.__index = Ray

-- コンストラクタ
-- @param origin Vec3 レイの原点
-- @param direction Vec3 レイの方向
function Ray.new(origin, direction)
    local self = setmetatable({}, Ray)
    self.origin = origin
    self.direction = direction
    return self
end

-- レイ上の点を計算
-- @param t number パラメータ
-- @return Vec3 P(t) = origin + t * direction
function Ray:at(t)
    return self.origin + self.direction * t
end

return Ray
