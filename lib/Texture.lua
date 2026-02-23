-- lib/Texture.lua
-- テクスチャサンプリングモジュール

local Texture = {}

-- テクスチャオブジェクトを生成
-- @param width number テクスチャ幅
-- @param height number テクスチャ高さ
-- @param channels number チャンネル数（3=RGB, 4=RGBA）
-- @param pixels table ピクセルデータ（1-indexed, R,G,B,... のフラット配列）
-- @return table テクスチャオブジェクト
function Texture.new(width, height, channels, pixels)
    local tex = {
        width = width,
        height = height,
        channels = channels,
        pixels = pixels
    }

    -- UV 座標からピクセル色を取得（最近傍サンプリング）
    -- @param u number U座標 [0, 1]
    -- @param v number V座標 [0, 1]
    -- @return number, number, number R, G, B (0~255)
    function tex:sample(u, v)
        -- ラップ処理（リピート）
        u = u % 1.0
        v = v % 1.0
        if u < 0 then u = u + 1.0 end
        if v < 0 then v = v + 1.0 end

        -- ピクセル座標に変換（最近傍）
        local px = math.floor(u * self.width)
        local py = math.floor(v * self.height)

        -- クランプ
        if px >= self.width then px = self.width - 1 end
        if py >= self.height then py = self.height - 1 end
        if px < 0 then px = 0 end
        if py < 0 then py = 0 end

        -- ピクセルデータのインデックス（1-indexed）
        local idx = (py * self.width + px) * self.channels + 1

        local r = self.pixels[idx] or 0
        local g = self.pixels[idx + 1] or 0
        local b = self.pixels[idx + 2] or 0

        return r, g, b
    end

    return tex
end

-- バリセントリック座標からUV座標を補間計算
-- @param texcoords table UV座標のフラット配列（1-indexed, u0,v0,u1,v1,...）
-- @param indices table インデックス配列（1-indexed, 値は0-indexed頂点番号）
-- @param primID number プリミティブID（0-indexed）
-- @param bary_u number バリセントリック座標U
-- @param bary_v number バリセントリック座標V
-- @return number, number 補間されたUV座標 (u, v)
function Texture.interpolate_uv(texcoords, indices, primID, bary_u, bary_v)
    -- 三角形の3頂点のインデックスを取得（indicesは1-indexed、値は0-indexed）
    local base = primID * 3
    local i0 = indices[base + 1]  -- 0-indexed頂点番号
    local i1 = indices[base + 2]
    local i2 = indices[base + 3]

    -- 各頂点のUV座標を取得（texcoordsは1-indexed、2成分ずつ）
    local u0 = texcoords[i0 * 2 + 1]
    local v0 = texcoords[i0 * 2 + 2]
    local u1 = texcoords[i1 * 2 + 1]
    local v1 = texcoords[i1 * 2 + 2]
    local u2 = texcoords[i2 * 2 + 1]
    local v2 = texcoords[i2 * 2 + 2]

    -- バリセントリック補間: P = (1-u-v)*P0 + u*P1 + v*P2
    local w = 1.0 - bary_u - bary_v
    local out_u = w * u0 + bary_u * u1 + bary_v * u2
    local out_v = w * v0 + bary_u * v1 + bary_v * v2

    return out_u, out_v
end

return Texture
