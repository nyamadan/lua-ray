-- scenes/cornell_box.lua
-- edupt の Cornell Box シーンを Lua で再現

local M = {}

-- モジュール
local Vec3 = require('lib.Vec3')
local Ray = require('lib.Ray')
local Material = require('lib.Material')
local Camera = require('lib.Camera')
local PathTracer = require('lib.PathTracer')
local BilateralFilter = require('lib.BilateralFilter')

-- モジュール内変数
local scene = nil
local camera = nil
local width = 0
local height = 0

-- マテリアルテーブル (geomID -> Material)
local materials = {}

-- 設定
local SAMPLES_PER_PIXEL = 32  -- サンプル数（品質重視）
local MAX_DEPTH = 10          -- レイの最大再帰深度

-- ===========================================
-- シーンインターフェース
-- ===========================================

-- シーンのセットアップ: 球体を追加
function M.setup(embree_scene, app_data)
    print("Setup Cornell Box Scene...")
    
    -- マテリアルデータ（JSONシリアライズ可能な形式）
    local material_data = {}
    local geomID
    
    -- Cornell Box の構成 (edupt scene.h より)
    -- 大きな球体で壁を表現
    local kBigRadius = 1e5
    
    -- 左壁（赤）
    geomID = embree_scene:add_sphere(kBigRadius + 1, 40.8, 81.6, kBigRadius)
    material_data[tostring(geomID)] = {type = "lambertian", albedo = {0.75, 0.25, 0.25}}
    
    -- 右壁（青）
    geomID = embree_scene:add_sphere(-kBigRadius + 99, 40.8, 81.6, kBigRadius)
    material_data[tostring(geomID)] = {type = "lambertian", albedo = {0.25, 0.25, 0.75}}
    
    -- 奥壁（灰）
    geomID = embree_scene:add_sphere(50, 40.8, kBigRadius, kBigRadius)
    material_data[tostring(geomID)] = {type = "lambertian", albedo = {0.75, 0.75, 0.75}}
    
    -- 手前（黒 = 見えない壁）
    geomID = embree_scene:add_sphere(50, 40.8, -kBigRadius + 250, kBigRadius)
    material_data[tostring(geomID)] = {type = "lambertian", albedo = {0.0, 0.0, 0.0}}
    
    -- 床（灰）
    geomID = embree_scene:add_sphere(50, kBigRadius, 81.6, kBigRadius)
    material_data[tostring(geomID)] = {type = "lambertian", albedo = {0.75, 0.75, 0.75}}
    
    -- 天井（灰）
    geomID = embree_scene:add_sphere(50, -kBigRadius + 81.6, 81.6, kBigRadius)
    material_data[tostring(geomID)] = {type = "lambertian", albedo = {0.75, 0.75, 0.75}}
    
    -- 緑の球体（拡散）
    geomID = embree_scene:add_sphere(65, 20, 20, 20)
    material_data[tostring(geomID)] = {type = "lambertian", albedo = {0.25, 0.75, 0.25}}
    
    -- 鏡の球体
    geomID = embree_scene:add_sphere(27, 16.5, 47, 16.5)
    material_data[tostring(geomID)] = {type = "metal", albedo = {0.99, 0.99, 0.99}, fuzz = 0.0}
    
    -- ガラスの球体
    geomID = embree_scene:add_sphere(77, 16.5, 78, 16.5)
    material_data[tostring(geomID)] = {type = "dielectric", ir = 1.5}
    
    -- 照明（発光球体）
    geomID = embree_scene:add_sphere(50.0, 90.0, 81.6, 15.0)
    material_data[tostring(geomID)] = {type = "diffuse_light", emit = {50, 50, 50}}
    
    -- JSONにシリアライズしてapp_dataに保存
    local json = require("lib.json")
    local json_str = json.encode(material_data)
    app_data:set_string("materials", json_str)
    
    print("Cornell Box setup complete. Material data saved to app_data")
end

-- シーンの開始: カメラとローカル変数の初期化
function M.start(embree_scene, app_data)
    print("Start Cornell Box Scene...")
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
            elseif data.type == "diffuse_light" then
                local emit = Vec3.new(data.emit[1], data.emit[2], data.emit[3])
                materials[geomID] = Material.DiffuseLight(emit)
            end
        end
        print("Materials deserialized successfully")
    else
        print("Warning: No materials found in app_data!")
    end
    
    -- カメラの作成 (edupt render.h より)
    -- position: (50, 52, 220)
    -- direction: (0, -0.04, -1)
    -- screen_dist: 40, screen_height: 30 -> FOV ≈ 41°
    camera = Camera.new("perspective", {
        position = {50, 52, 220},
        look_at = {50, 52 - 0.04 * 40, 220 - 40},  -- position + direction * screen_dist
        up = {0, 1, 0},
        aspect_ratio = aspect_ratio,
        fov = 41.0
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
        
        -- パストレーシングで放射輝度を計算
        color = color + PathTracer.radiance(ray, scene, materials, MAX_DEPTH)
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
function M.post_effect(data, x, y)
    local r, g, b = BilateralFilter.filter(data, x, y)
    data:set_pixel(x, y, r, g, b)
end

return M
