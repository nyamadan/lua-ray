-- lib/PathTracer.lua
-- パストレーシング用モジュール (edupt 移植)

local Vec3 = require('lib.Vec3')
local Ray = require('lib.Ray')

local PathTracer = {}
local BSDF = require('lib.BSDF')

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

local function table_vec(x, y, z) return {x, y, z} end
local function mul(a, b) return {a[1]*b[1], a[2]*b[2], a[3]*b[3]} end
local function scale(a, value) return {a[1]*value, a[2]*value, a[3]*value} end
local function add_to(target, value)
    target[1], target[2], target[3] = target[1]+value[1], target[2]+value[2], target[3]+value[3]
end
local function dot_table(a, b) return a[1]*b[1]+a[2]*b[2]+a[3]*b[3] end
local function normalize_table(v)
    local length = math.sqrt(math.max(dot_table(v, v), 1e-12))
    return {v[1]/length, v[2]/length, v[3]/length}
end
local function nonzero(v) return v and (v[1] > 0 or v[2] > 0 or v[3] > 0) end

-- Surface-only path tracing with next-event estimation and power-heuristic MIS.
-- callbacks.surface(hit, incoming_direction) returns {material, normal, emission}.
-- callbacks.sample_light(point, rng) returns {direction, distance, radiance, pdf}.
-- callbacks.light_pdf(previous_point, light_hit) returns a solid-angle PDF.
function PathTracer.trace_mis(ray, scene, callbacks, rng, options)
    options = options or {}
    local max_depth = options.max_depth or 8
    local rr_depth = options.rr_depth or 4
    local radiance, throughput = {0,0,0}, {1,1,1}
    local previous_pdf, previous_delta, previous_point = 0, true, nil
    local current_origin = {ray.origin.x, ray.origin.y, ray.origin.z}
    local current_direction = normalize_table({ray.direction.x, ray.direction.y, ray.direction.z})

    for depth=0,max_depth-1 do
        local hit_ok, distance, nx, ny, nz, geom_id, prim_id, bary_u, bary_v = scene:intersect(
            current_origin[1], current_origin[2], current_origin[3],
            current_direction[1], current_direction[2], current_direction[3])
        if not hit_ok then
            local background = callbacks.background and callbacks.background(current_direction) or {0,0,0}
            add_to(radiance, mul(throughput, background))
            break
        end

        local point = {current_origin[1]+current_direction[1]*distance,
                       current_origin[2]+current_direction[2]*distance,
                       current_origin[3]+current_direction[3]*distance}
        local geometric_normal = normalize_table({nx,ny,nz})
        if dot_table(geometric_normal, current_direction) > 0 then geometric_normal = scale(geometric_normal, -1) end
        local hit = {point=point, geometric_normal=geometric_normal, geom_id=geom_id,
                     prim_id=prim_id, bary_u=bary_u, bary_v=bary_v}
        local surface = callbacks.surface(hit, current_direction)
        local normal = normalize_table(surface.normal or geometric_normal)
        if dot_table(normal, current_direction) > 0 then normal = scale(normal, -1) end

        if nonzero(surface.emission) then
            local weight = 1
            if depth > 0 and not previous_delta and callbacks.light_pdf then
                weight = BSDF.power_heuristic(previous_pdf, callbacks.light_pdf(previous_point, hit) or 0)
            end
            add_to(radiance, scale(mul(throughput, surface.emission), weight))
        end
        if not surface.material then break end

        local wo = scale(current_direction, -1)
        if callbacks.sample_light then
            local light = callbacks.sample_light(point, rng)
            if light and light.pdf and light.pdf > 0 and dot_table(normal, light.direction) > 0 then
                local blocked = false
                local shadow_hit, shadow_distance = scene:intersect(
                    point[1]+geometric_normal[1]*1e-4, point[2]+geometric_normal[2]*1e-4,
                    point[3]+geometric_normal[3]*1e-4,
                    light.direction[1], light.direction[2], light.direction[3])
                if shadow_hit and shadow_distance < light.distance-2e-4 then blocked = true end
                if not blocked then
                    local f = BSDF.evaluate(surface.material, normal, wo, light.direction)
                    local bsdf_pdf = BSDF.pdf(surface.material, normal, wo, light.direction)
                    local weight = BSDF.power_heuristic(light.pdf, bsdf_pdf)
                    local contribution = scale(mul(mul(throughput, f), light.radiance),
                        dot_table(normal, light.direction)*weight/light.pdf)
                    add_to(radiance, contribution)
                end
            end
        end

        local wi, f, pdf, is_delta = BSDF.sample(surface.material, normal, wo, rng)
        local cosine = math.max(dot_table(normal, wi), 0)
        if pdf <= 0 or cosine <= 0 then break end
        throughput = scale(mul(throughput, f), cosine/pdf)
        if not nonzero(throughput) then break end

        if depth+1 >= rr_depth then
            local survival = math.min(0.95, math.max(throughput[1], throughput[2], throughput[3]))
            if rng:next() >= survival then break end
            throughput = scale(throughput, 1/survival)
        end
        previous_pdf, previous_delta, previous_point = pdf, is_delta, point
        current_origin = {point[1]+geometric_normal[1]*1e-4,
                          point[2]+geometric_normal[2]*1e-4,
                          point[3]+geometric_normal[3]*1e-4}
        current_direction = wi
    end
    return radiance
end

return PathTracer
