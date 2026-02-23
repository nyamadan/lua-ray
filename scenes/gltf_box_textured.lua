-- scenes/gltf_box_textured.lua
-- テクスチャ付きglTFボックスモデルを読み込んで表示するシーン
-- setup/start が異なるスレッドで実行されても動作するよう、
-- app_data のキャッシュ機構を使用してデータを共有する

local Texture = require("lib.Texture")

local M = {}

-- モジュール内でシーン、カメラ、サイズを保持
local scene = nil
local camera = nil
local width = 0
local height = 0

-- テクスチャ関連データ（各スレッドの start で初期化）
local textures = {}    -- テクスチャオブジェクト配列（テクスチャインデックス → Texture）
local texcoords = nil  -- UV座標（フラット配列）
local indices = nil    -- インデックス配列

-- glTF キャッシュ名
local GLTF_NAME = "box_textured"
local GLTF_PATH = "assets/BoxTextured.glb"

-- ライト方向（XYZ = 1, 2, 3 を正規化）
local lx, ly, lz = 1.0, 2.0, 3.0
local llen = math.sqrt(lx*lx + ly*ly + lz*lz)
local lightDirX, lightDirY, lightDirZ = lx/llen, ly/llen, lz/llen

-- シーンのセットアップ: ジオメトリのみ追加（メインスレッドで実行）
function M.setup(embree_scene, app_data)
    print("Setup glTF Box Textured Scene...")

    -- glTF をロードして app_data にキャッシュ
    local ok = app_data:load_gltf(GLTF_NAME, GLTF_PATH)
    if not ok then
        print("Error: " .. GLTF_PATH .. " の読み込みに失敗しました")
        return
    end

    local mesh_count = app_data:get_gltf_mesh_count(GLTF_NAME)
    print("glTF loaded: " .. mesh_count .. " mesh(es)")

    -- テクスチャ画像をデコードしてキャッシュ（複数テクスチャ対応）
    -- BoxTextured.glb にはテクスチャが1つだが、汎用的にループ
    local tex_idx = 0
    while true do
        local tex_name = GLTF_NAME .. "_tex" .. tex_idx
        local loaded = app_data:load_texture_image(tex_name, GLTF_NAME, tex_idx)
        if not loaded then break end
        print("  Texture " .. tex_idx .. " cached as '" .. tex_name .. "'")
        tex_idx = tex_idx + 1
    end

    -- 各メッシュのプリミティブをシーンに追加
    for i = 0, mesh_count - 1 do
        local vertices = app_data:get_gltf_vertices(GLTF_NAME, i, 0)
        local mesh_indices = app_data:get_gltf_indices(GLTF_NAME, i, 0)
        if vertices and mesh_indices and #vertices > 0 and #mesh_indices > 0 then
            local geomID = embree_scene:add_mesh(vertices, mesh_indices)
            print("  Mesh " .. i .. ": " .. (#vertices / 3) .. " vertices, " .. (#mesh_indices / 3) .. " triangles -> geomID=" .. geomID)
        end
    end

    print("glTF Box Textured Scene setup complete!")
end

-- シーンの開始: テクスチャ・UV データを app_data から取得（各スレッドで実行）
function M.start(embree_scene, app_data)
    print("Start glTF Box Textured Scene...")
    scene = embree_scene
    width = app_data:width()
    height = app_data:height()
    local aspect_ratio = width / height

    -- app_data からテクスチャ画像を取得して Texture オブジェクトを生成
    textures = {}
    local tex_idx = 0
    while true do
        local tex_name = GLTF_NAME .. "_tex" .. tex_idx
        local image = app_data:get_texture_image(tex_name)
        if not image then break end
        textures[tex_idx] = Texture.new(image.width, image.height, image.channels, image.pixels)
        print("  Texture " .. tex_idx .. ": " .. image.width .. "x" .. image.height .. " loaded from cache")
        tex_idx = tex_idx + 1
    end

    -- app_data から UV 座標とインデックスを取得
    texcoords = app_data:get_gltf_tex_coords(GLTF_NAME, 0, 0)
    indices = app_data:get_gltf_indices(GLTF_NAME, 0, 0)

    if texcoords then
        print("  UV coords: " .. #texcoords .. " floats (" .. (#texcoords / 2) .. " vertices)")
    else
        print("  Warning: UV座標が見つかりませんでした")
    end

    local CameraUtils = require("lib.CameraUtils")
    camera = CameraUtils.setup_or_sync_camera(camera, app_data, {
        position = {2.0, 1.5, 3.0},
        look_at = {0, 0, 0},
        up = {0, 1, 0},
        aspect_ratio = aspect_ratio,
        fov = 45.0
    })
    print("Camera synchronized for glTF Box Textured Scene")
end

-- ピクセルの色を計算（テクスチャマッピング + ディフューズシェーディング）
function M.shade(data, x, y)
    -- 整数座標から正規化座標 [-1, 1] を計算
    local u = (2.0 * x - width) / width
    local v = (2.0 * y - height) / height

    -- カメラからレイを生成
    local ox, oy, oz, dx, dy, dz = camera:generate_ray(u, v)

    -- シーンとのインターセクト判定（バリセントリック座標付き）
    local hit, t, nx, ny, nz, geomID, primID, baryU, baryV = scene:intersect(ox, oy, oz, dx, dy, dz)

    -- Y座標を上下反転（画像座標系からテクスチャ座標系への変換）
    local flip_y = height - 1 - y

    if hit then
        local r, g, b = 200, 200, 200  -- デフォルト色（グレー）

        -- テクスチャサンプリング（テクスチャ0を使用、複数テクスチャの場合は拡張可能）
        local tex = textures[0]
        if tex and texcoords and indices then
            -- バリセントリック座標からUV座標を補間
            local tex_u, tex_v = Texture.interpolate_uv(texcoords, indices, primID, baryU, baryV)
            -- テクスチャからピクセル色を取得
            r, g, b = tex:sample(tex_u, tex_v)
        end

        -- ディフューズシェーディング計算
        local diffuse = nx * lightDirX + ny * lightDirY + nz * lightDirZ
        if diffuse < 0 then diffuse = 0 end

        -- アンビエント光を追加
        diffuse = 0.15 + 0.85 * diffuse
        if diffuse > 1.0 then diffuse = 1.0 end

        -- テクスチャ色にシェーディングを適用
        r = math.floor(r * diffuse)
        g = math.floor(g * diffuse)
        b = math.floor(b * diffuse)

        -- クランプ
        if r > 255 then r = 255 end
        if g > 255 then g = 255 end
        if b > 255 then b = 255 end

        data:set_pixel(x, flip_y, r, g, b)
    else
        -- グラデーション背景（空のような）
        local bg_t = (v + 1.0) * 0.5
        local bg_r = math.floor(255 * (1.0 - bg_t) * 0.6 + 255 * bg_t * 0.2)
        local bg_g = math.floor(255 * (1.0 - bg_t) * 0.7 + 255 * bg_t * 0.3)
        local bg_b = math.floor(255 * (1.0 - bg_t) * 0.9 + 255 * bg_t * 0.6)
        data:set_pixel(x, flip_y, bg_r, bg_g, bg_b)
    end
end

-- 外部からカメラインスタンスを取得できるようにする
function M.get_camera()
    return camera
end

-- クリーンアップ処理
function M.cleanup()
    camera = nil
    textures = {}
    texcoords = nil
    indices = nil
end

return M
