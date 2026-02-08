-- lib/PathTracer.lua
-- パストレーシング用モジュール (edupt 移植)

local Vec3 = require('lib.Vec3')
local Ray = require('lib.Ray')

local PathTracer = {}

-- 定数
PathTracer.kBackgroundColor = Vec3.new(0, 0, 0)  -- 背景色（黒）
PathTracer.kDepth = 5           -- ロシアンルーレットを開始するまでの最小深度
PathTracer.kDepthLimit = 64     -- 再帰の最大深度
PathTracer.kEPS = 1e-6          -- 浮動小数点誤差許容値
PathTracer.kPI = math.pi

-- ===========================================
-- 正規直交基底を生成
-- ===========================================
-- 法線wに対して直交するu, vを生成
function PathTracer.create_orthonormal_basis(normal)
    local w = normal:normalize()
    
    local up
    if math.abs(w.x) > PathTracer.kEPS then
        up = Vec3.new(0, 1, 0)
    else
        up = Vec3.new(1, 0, 0)
    end
    
    local u = Vec3.cross(up, w):normalize()
    local v = Vec3.cross(w, u)
    
    return w, u, v
end

-- ===========================================
-- コサイン重点サンプリング
-- ===========================================
-- 法線方向を基準とした半球上のランダム方向を生成
function PathTracer.cosine_weighted_sample(normal)
    local w, u, v = PathTracer.create_orthonormal_basis(normal)
    
    local r1 = 2 * PathTracer.kPI * math.random()
    local r2 = math.random()
    local r2s = math.sqrt(r2)
    
    local dir = u * math.cos(r1) * r2s +
                v * math.sin(r1) * r2s +
                w * math.sqrt(1.0 - r2)
    
    return dir:normalize()
end

-- ===========================================
-- radiance関数 (放射輝度を計算)
-- ===========================================
-- edupt の radiance 関数を移植
-- @param ray Ray レイ
-- @param scene EmbreeScene シーン
-- @param materials table マテリアルテーブル (geomID -> Material)
-- @param depth int 現在の再帰深度
-- @return Vec3 放射輝度
function PathTracer.radiance(ray, scene, materials, depth, max_depth)
    -- max_depth が渡されない場合は depth を使用（初回呼び出し）
    max_depth = max_depth or depth
    
    -- 深度が0以下の場合は黒を返す
    if depth <= 0 then
        return Vec3.new(0, 0, 0)
    end
    
    -- シーンとの交差判定
    local ox, oy, oz = ray.origin.x, ray.origin.y, ray.origin.z
    local dx, dy, dz = ray.direction.x, ray.direction.y, ray.direction.z
    local hit, t, nx, ny, nz, geomID, primID = scene:intersect(ox, oy, oz, dx, dy, dz)
    
    if not hit then
        return PathTracer.kBackgroundColor
    end
    
    -- ヒット情報
    local hitpoint = ray:at(t)
    local normal = Vec3.new(nx, ny, nz)
    
    -- 表面の向き（内外判定）
    local front_face = Vec3.dot(ray.direction, normal) < 0
    local orienting_normal = front_face and normal or (-normal)
    
    -- マテリアル取得
    local material = materials[geomID]
    if not material then
        return Vec3.new(1, 0, 1)  -- マゼンタ（デバッグ用）
    end
    
    -- 発光を取得
    local emission = Vec3.new(0, 0, 0)
    if material.emitted then
        emission = material:emitted()
    end
    
    -- HitRecordを構築
    local rec = {
        p = hitpoint,
        normal = orienting_normal,
        t = t,
        front_face = front_face
    }
    
    -- 散乱
    local scattered, attenuation = material:scatter(ray, rec)
    
    if not scattered then
        -- 散乱しない（光源など）
        return emission
    end
    
    -- 反射率の最大値（ロシアンルーレット用）
    local russian_roulette_probability = math.max(attenuation.x, math.max(attenuation.y, attenuation.z))
    
    -- 深度が制限を超えた場合は確率を急激に下げる
    if depth > PathTracer.kDepthLimit then
        russian_roulette_probability = russian_roulette_probability * (0.5 ^ (depth - PathTracer.kDepthLimit))
    end
    
    -- ロシアンルーレット
    -- 経過した深度 = max_depth - depth
    local elapsed_depth = max_depth - depth
    
    if elapsed_depth > PathTracer.kDepth then
        if math.random() >= russian_roulette_probability then
            return emission
        end
    else
        russian_roulette_probability = 1.0
    end
    
    -- 再帰的にレイを追跡
    local incoming_radiance = PathTracer.radiance(scattered, scene, materials, depth - 1, max_depth)
    
    -- weight = attenuation / russian_roulette_probability
    local weight = attenuation / russian_roulette_probability
    
    -- emission + weight * incoming_radiance
    return emission + Vec3.new(
        weight.x * incoming_radiance.x,
        weight.y * incoming_radiance.y,
        weight.z * incoming_radiance.z
    )
end

return PathTracer
