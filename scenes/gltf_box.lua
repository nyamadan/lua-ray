-- scenes/gltf_box.lua
-- glTFボックスモデルを読み込んで表示するサンプルシーン

local M = {}

-- モジュール内でシーン、カメラ、サイズを保持
local scene = nil
local camera = nil
local width = 0
local height = 0

-- ライト方向（XYZ = 1, 2, 3 を正規化）
local lx, ly, lz = 1.0, 2.0, 3.0
local llen = math.sqrt(lx*lx + ly*ly + lz*lz)
local lightDirX, lightDirY, lightDirZ = lx/llen, ly/llen, lz/llen

-- シーンのセットアップ: glTFファイルからメッシュを読み込み
function M.setup(embree_scene, app_data)
    print("Setup glTF Box Scene...")

    -- glTFファイルを読み込む
    local gltf = GltfData.new()
    local ok = gltf:load("assets/Box.glb")
    if not ok then
        print("Error: assets/Box.glb の読み込みに失敗しました")
        return
    end

    print("glTF loaded: " .. gltf:get_mesh_count() .. " mesh(es)")

    -- 各メッシュのプリミティブをシーンに追加
    for i = 0, gltf:get_mesh_count() - 1 do
        local vertices = gltf:get_vertices(i, 0)
        local indices = gltf:get_indices(i, 0)
        if #vertices > 0 and #indices > 0 then
            local geomID = embree_scene:add_mesh(vertices, indices)
            print("  Mesh " .. i .. ": " .. (#vertices / 3) .. " vertices, " .. (#indices / 3) .. " triangles -> geomID=" .. geomID)
        end
    end

    print("glTF Box Scene setup complete!")
end

-- シーンの開始: カメラとローカル変数の初期化
function M.start(embree_scene, app_data)
    print("Start glTF Box Scene...")
    scene = embree_scene
    width = app_data:width()
    height = app_data:height()
    local aspect_ratio = width / height

    -- カメラの作成: 透視投影、ボックスが見える位置に配置
    local Camera = require("lib.Camera")
    camera = Camera.new("perspective", {
        position = {2.0, 1.5, 3.0},
        look_at = {0, 0, 0},
        up = {0, 1, 0},
        aspect_ratio = aspect_ratio,
        fov = 45.0
    })
    print("Camera created for glTF Box Scene")
end

-- ピクセルの色を計算（ディフューズシェーディング）
function M.shade(data, x, y)
    -- 整数座標から正規化座標 [-1, 1] を計算
    local u = (2.0 * x - width) / width
    local v = (2.0 * y - height) / height

    -- カメラからレイを生成
    local ox, oy, oz, dx, dy, dz = camera:generate_ray(u, v)

    -- シーンとのインターセクト判定
    local hit, t, nx, ny, nz = scene:intersect(ox, oy, oz, dx, dy, dz)

    -- Y座標を上下反転（画像座標系からテクスチャ座標系への変換）
    local flip_y = height - 1 - y

    if hit then
        -- ディフューズシェーディング計算
        local diffuse = nx * lightDirX + ny * lightDirY + nz * lightDirZ
        if diffuse < 0 then diffuse = 0 end

        -- アンビエント光を追加
        diffuse = 0.15 + 0.85 * diffuse
        if diffuse > 1.0 then diffuse = 1.0 end

        local shade = math.floor(255 * diffuse)
        -- ブルーグレーのティント
        data:set_pixel(x, flip_y, math.floor(shade * 0.7), math.floor(shade * 0.8), shade)
    else
        -- グラデーション背景（空のような）
        local bg_t = (v + 1.0) * 0.5
        local r = math.floor(255 * (1.0 - bg_t) * 0.6 + 255 * bg_t * 0.2)
        local g = math.floor(255 * (1.0 - bg_t) * 0.7 + 255 * bg_t * 0.3)
        local b = math.floor(255 * (1.0 - bg_t) * 0.9 + 255 * bg_t * 0.6)
        data:set_pixel(x, flip_y, r, g, b)
    end
end

return M
