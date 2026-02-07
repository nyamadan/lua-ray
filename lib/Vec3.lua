-- lib/Vec3.lua
-- 3Dベクトルクラス (Ray Tracing in One Weekend用)

local Vec3 = {}
Vec3.__index = Vec3

-- コンストラクタ
function Vec3.new(x, y, z)
    local self = setmetatable({}, Vec3)
    self.x = x or 0
    self.y = y or 0
    self.z = z or 0
    return self
end

-- 演算子オーバーロード: 加算
function Vec3.__add(a, b)
    return Vec3.new(a.x + b.x, a.y + b.y, a.z + b.z)
end

-- 演算子オーバーロード: 減算
function Vec3.__sub(a, b)
    return Vec3.new(a.x - b.x, a.y - b.y, a.z - b.z)
end

-- 演算子オーバーロード: 乗算 (スカラーまたはベクトル)
function Vec3.__mul(a, b)
    if type(a) == "number" then
        return Vec3.new(a * b.x, a * b.y, a * b.z)
    elseif type(b) == "number" then
        return Vec3.new(a.x * b, a.y * b, a.z * b)
    else
        -- ベクトル同士の要素ごとの乗算
        return Vec3.new(a.x * b.x, a.y * b.y, a.z * b.z)
    end
end

-- 演算子オーバーロード: 除算 (スカラー)
function Vec3.__div(a, b)
    return Vec3.new(a.x / b, a.y / b, a.z / b)
end

-- 演算子オーバーロード: 単項マイナス
function Vec3.__unm(a)
    return Vec3.new(-a.x, -a.y, -a.z)
end

-- 長さの二乗
function Vec3:length_squared()
    return self.x * self.x + self.y * self.y + self.z * self.z
end

-- 長さ
function Vec3:length()
    return math.sqrt(self:length_squared())
end

-- 正規化
function Vec3:normalize()
    local len = self:length()
    if len > 0 then
        return Vec3.new(self.x / len, self.y / len, self.z / len)
    end
    return Vec3.new(0, 0, 0)
end

-- ドット積
function Vec3.dot(a, b)
    return a.x * b.x + a.y * b.y + a.z * b.z
end

-- クロス積
function Vec3.cross(a, b)
    return Vec3.new(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    )
end

-- 反射ベクトル
function Vec3.reflect(v, n)
    return v - n * (2 * Vec3.dot(v, n))
end

-- ゼロに近いかどうか
function Vec3:near_zero()
    local s = 1e-8
    return math.abs(self.x) < s and math.abs(self.y) < s and math.abs(self.z) < s
end

-- 単位球内のランダムベクトル
function Vec3.random_in_unit_sphere()
    while true do
        local p = Vec3.new(
            math.random() * 2 - 1,
            math.random() * 2 - 1,
            math.random() * 2 - 1
        )
        if p:length_squared() < 1 then
            return p
        end
    end
end

-- 単位ベクトル（単位球面上のランダム点）
function Vec3.random_unit_vector()
    return Vec3.random_in_unit_sphere():normalize()
end

-- 半球内のランダムベクトル
function Vec3.random_in_hemisphere(normal)
    local in_unit_sphere = Vec3.random_in_unit_sphere()
    if Vec3.dot(in_unit_sphere, normal) > 0.0 then
        return in_unit_sphere
    else
        return -in_unit_sphere
    end
end

-- 単位円盤内のランダムベクトル（被写界深度用）
function Vec3.random_in_unit_disk()
    while true do
        local p = Vec3.new(math.random() * 2 - 1, math.random() * 2 - 1, 0)
        if p:length_squared() < 1 then
            return p
        end
    end
end

-- 屈折ベクトル（スネルの法則）
function Vec3.refract(uv, n, etai_over_etat)
    local cos_theta = math.min(Vec3.dot(-uv, n), 1.0)
    local r_out_perp = (uv + n * cos_theta) * etai_over_etat
    local r_out_parallel = n * (-math.sqrt(math.abs(1.0 - r_out_perp:length_squared())))
    return r_out_perp + r_out_parallel
end

return Vec3
