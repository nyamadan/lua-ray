-- scenes/raytracing_weekend.lua
-- Ray Tracing in One Weekend
-- original: https://github.com/RayTracing/raytracing.github.io

local M = {}

-- モジュール
local Vec3 = require('lib.Vec3')
local Ray = require('lib.Ray')
local Material = require('lib.Material')
local Camera = require('lib.Camera')
local BilateralFilter = require('lib.BilateralFilter')

-- モジュール内変数
local scene = nil
local camera = nil
local width = 0
local height = 0

-- マテリアルテーブル (geomID -> Material)
local materials = {}

-- 設定
local SAMPLES_PER_PIXEL = 10  -- アンチエイリアシング用サンプル数
local MAX_DEPTH = 10          -- レイの最大再帰深度

-- ===========================================
-- ヘルパー関数
-- ===========================================

-- 範囲内のランダム数
local function random_double(min, max)
    min = min or 0
    max = max or 1
    return min + (max - min) * math.random()
end

-- レイの色を計算（再帰的Path Tracing）
local function ray_color(r, embree_scene, depth)
    -- 再帰の深さを超えた場合は黒を返す
    if depth <= 0 then
        return Vec3.new(0, 0, 0)
    end
    
    -- シーンとのインターセクト判定
    local ox, oy, oz = r.origin.x, r.origin.y, r.origin.z
    local dx, dy, dz = r.direction.x, r.direction.y, r.direction.z
    local hit, t, nx, ny, nz, geomID, primID = embree_scene:intersect(ox, oy, oz, dx, dy, dz)
    
    if hit then
        -- ヒットポイント
        local p = r:at(t)
        local normal = Vec3.new(nx, ny, nz)
        
        -- 表面の向き判定
        local front_face = Vec3.dot(r.direction, normal) < 0
        if not front_face then
            normal = -normal
        end
        
        -- HitRecordを構築
        local rec = {
            p = p,
            normal = normal,
            t = t,
            front_face = front_face
        }
        
        -- マテリアル取得
        local material = materials[geomID]
        if material then
            local scattered, attenuation = material:scatter(r, rec)
            if scattered then
                -- 再帰的にレイを追跡
                local color = ray_color(scattered, embree_scene, depth - 1)
                return Vec3.new(
                    attenuation.x * color.x,
                    attenuation.y * color.y,
                    attenuation.z * color.z
                )
            end
            return Vec3.new(0, 0, 0)
        else
            -- マテリアルがない場合はマゼンタ
            return Vec3.new(1, 0, 1)
        end
    end
    
    -- 背景（空のグラデーション）
    local unit_direction = r.direction:normalize()
    local t_bg = 0.5 * (unit_direction.y + 1.0)
    local white = Vec3.new(1.0, 1.0, 1.0)
    local blue = Vec3.new(0.5, 0.7, 1.0)
    return white * (1.0 - t_bg) + blue * t_bg
end

-- ===========================================
-- シーンインターフェース
-- ===========================================

-- シーンのセットアップ: 球体を追加
function M.setup(embree_scene, app_data)
    print("Setup Ray Tracing Weekend Scene...")
    
    -- マテリアルデータ（JSONシリアライズ可能な形式）
    local material_data = {}
    local geomID
    
    -- 地面（大きな球体）
    geomID = embree_scene:add_sphere(0, -1000, 0, 1000)
    material_data[tostring(geomID)] = {type = "lambertian", albedo = {0.5, 0.5, 0.5}}
    
    -- 中央の3つの球体
    -- 1. ガラス球（中央左）
    geomID = embree_scene:add_sphere(0, 1, 0, 1.0)
    material_data[tostring(geomID)] = {type = "dielectric", ir = 1.5}
    
    -- 2. 拡散反射球（中央）
    geomID = embree_scene:add_sphere(-4, 1, 0, 1.0)
    material_data[tostring(geomID)] = {type = "lambertian", albedo = {0.4, 0.2, 0.1}}
    
    -- 3. 金属球（中央右）
    geomID = embree_scene:add_sphere(4, 1, 0, 1.0)
    material_data[tostring(geomID)] = {type = "metal", albedo = {0.7, 0.6, 0.5}, fuzz = 0.0}
    
    -- ランダムな小さい球体
    for a = -11, 10 do
        for b = -11, 10 do
            local choose_mat = math.random()
            local cx = a + 0.9 * math.random()
            local cy = 0.2
            local cz = b + 0.9 * math.random()
            
            -- 中央の大きな球体と重ならないようにする
            local dx = cx - 4
            local dy = cy - 0.2
            local dz = cz - 0
            local dist_from_center = math.sqrt(dx*dx + dy*dy + dz*dz)
            
            if dist_from_center > 0.9 then
                geomID = embree_scene:add_sphere(cx, cy, cz, 0.2)
                
                if choose_mat < 0.8 then
                    -- 拡散反射
                    local r = math.random() * math.random()
                    local g = math.random() * math.random()
                    local b = math.random() * math.random()
                    material_data[tostring(geomID)] = {type = "lambertian", albedo = {r, g, b}}
                elseif choose_mat < 0.95 then
                    -- 金属
                    local r = random_double(0.5, 1)
                    local g = random_double(0.5, 1)
                    local b = random_double(0.5, 1)
                    local fuzz = random_double(0, 0.5)
                    material_data[tostring(geomID)] = {type = "metal", albedo = {r, g, b}, fuzz = fuzz}
                else
                    -- ガラス
                    material_data[tostring(geomID)] = {type = "dielectric", ir = 1.5}
                end
            end
        end
    end
    
    -- JSONにシリアライズしてapp_dataに保存
    local json = require("lib.json")
    local json_str = json.encode(material_data)
    app_data:set_string("materials", json_str)
    
    print("Scene setup complete. Material data saved to app_data")
end

-- シーンの開始: カメラとローカル変数の初期化
function M.start(embree_scene, app_data)
    print("Start Ray Tracing Weekend Scene...")
    scene = embree_scene
    width = app_data:width()
    height = app_data:height()
    local aspect_ratio = width / height
    
    -- app_dataからマテリアルデータを取得してMaterialオブジェクトを再構築
    materials = {}
    local json = require("lib.json")
    local json_str = app_data:get_string("materials")
    
    if json_str and json_str ~= "" then
        local material_data = json.decode(json_str)
        for key, data in pairs(material_data) do
            local geomID = tonumber(key)
            if data.type == "lambertian" then
                local albedo = Vec3.new(data.albedo[1], data.albedo[2], data.albedo[3])
                materials[geomID] = Material.Lambertian(albedo)
            elseif data.type == "metal" then
                local albedo = Vec3.new(data.albedo[1], data.albedo[2], data.albedo[3])
                materials[geomID] = Material.Metal(albedo, data.fuzz)
            elseif data.type == "dielectric" then
                materials[geomID] = Material.Dielectric(data.ir)
            end
        end
        print("Materials deserialized successfully")
    else
        print("Warning: No materials found in app_data!")
    end
    
    -- カメラの作成: 透視投影
    local CameraUtils = require("lib.CameraUtils")
    camera = CameraUtils.setup_or_sync_camera(camera, app_data, {
        position = {13, 2, 3},
        look_at = {0, 0, 0},
        up = {0, 1, 0},
        aspect_ratio = aspect_ratio,
        fov = 20.0
    })
    
    print("Camera initialized")
end

-- ピクセルの色を計算（パストレーシング）
function M.shade(data, x, y)
    local color = Vec3.new(0, 0, 0)
    
    -- アンチエイリアシング: 複数サンプルの平均
    for s = 1, SAMPLES_PER_PIXEL do
        -- ピクセル内のランダムなオフセット
        local u = (2.0 * (x + math.random()) - width) / width
        local v = (2.0 * (y + math.random()) - height) / height
        
        -- カメラからレイを生成
        local ox, oy, oz, dx, dy, dz = camera:generate_ray(u, v)
        local origin = Vec3.new(ox, oy, oz)
        local direction = Vec3.new(dx, dy, dz)
        local ray = Ray.new(origin, direction)
        
        -- レイの色を計算
        color = color + ray_color(ray, scene, MAX_DEPTH)
    end
    
    -- サンプル平均
    local scale = 1.0 / SAMPLES_PER_PIXEL
    local r = math.sqrt(color.x * scale)  -- ガンマ補正 (gamma = 2)
    local g = math.sqrt(color.y * scale)
    local b = math.sqrt(color.z * scale)
    
    -- クランプ
    r = math.min(1.0, math.max(0.0, r))
    g = math.min(1.0, math.max(0.0, g))
    b = math.min(1.0, math.max(0.0, b))
    
    -- Y座標を上下反転
    local flip_y = height - 1 - y
    
    data:set_pixel(x, flip_y, math.floor(255 * r), math.floor(255 * g), math.floor(255 * b))
end

-- ポストエフェクト: バイラテラルフィルタによるノイズ低減
-- フロントバッファから読み取り、バックバッファに書き込む
function M.post_effect(data, x, y)
    local r, g, b = BilateralFilter.filter(data, x, y)
    data:set_pixel(x, y, r, g, b)
end

-- 外部からカメラインスタンスを取得できるようにする
function M.get_camera()
    return camera
end

-- クリーンアップ処理
function M.cleanup()
    camera = nil -- シーンリセット時にカメラも完全に初期化させる
end

return M
