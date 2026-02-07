-- lib/BilateralFilter.lua
-- バイラテラルフィルタ モジュール

local M = {}

-- デフォルトパラメータ
M.RADIUS = 3              -- フィルタ半径
M.SIGMA_SPATIAL = 2.0     -- 空間の標準偏差
M.SIGMA_COLOR = 0.1       -- 色の標準偏差 (0-1正規化)

-- ガウス関数
-- @param x 入力値
-- @param sigma 標準偏差
-- @return ガウス分布の値
function M.gaussian(x, sigma)
    return math.exp(-(x * x) / (2 * sigma * sigma))
end

-- バイラテラルフィルタ（1ピクセル処理）
-- フロントバッファから読み取り、フィルタ結果を返す
-- @param data AppDataインスタンス（フロントバッファから読み取る）
-- @param x 処理対象のX座標
-- @param y 処理対象のY座標
-- @param params オプションパラメータ {radius, sigma_spatial, sigma_color}
-- @return r, g, b フィルタ後のピクセル値 (0-255)
function M.filter(data, x, y, params)
    params = params or {}
    local radius = params.radius or M.RADIUS
    local sigma_spatial = params.sigma_spatial or M.SIGMA_SPATIAL
    local sigma_color = params.sigma_color or M.SIGMA_COLOR
    
    local width = data:width()
    local height = data:height()
    
    -- 中心ピクセルの色を取得
    local cr, cg, cb = data:get_pixel(x, y)
    cr, cg, cb = cr / 255, cg / 255, cb / 255
    
    local sum_r, sum_g, sum_b = 0, 0, 0
    local weight_sum = 0
    
    for dy = -radius, radius do
        for dx = -radius, radius do
            local nx, ny = x + dx, y + dy
            
            -- 境界チェック
            if nx >= 0 and nx < width and ny >= 0 and ny < height then
                local nr, ng, nb = data:get_pixel(nx, ny)
                nr, ng, nb = nr / 255, ng / 255, nb / 255
                
                -- 空間重み（ピクセル間の距離に基づく）
                local spatial_dist = math.sqrt(dx * dx + dy * dy)
                local spatial_weight = M.gaussian(spatial_dist, sigma_spatial)
                
                -- 色差重み（色の類似度に基づく）
                local color_diff = math.sqrt(
                    (nr - cr)^2 + (ng - cg)^2 + (nb - cb)^2
                )
                local color_weight = M.gaussian(color_diff, sigma_color)
                
                -- 合計重み
                local weight = spatial_weight * color_weight
                
                sum_r = sum_r + nr * weight
                sum_g = sum_g + ng * weight
                sum_b = sum_b + nb * weight
                weight_sum = weight_sum + weight
            end
        end
    end
    
    -- 正規化して返す
    if weight_sum > 0 then
        return 
            math.floor((sum_r / weight_sum) * 255),
            math.floor((sum_g / weight_sum) * 255),
            math.floor((sum_b / weight_sum) * 255)
    else
        -- weight_sumが0の場合は元の色をそのまま返す
        return math.floor(cr * 255), math.floor(cg * 255), math.floor(cb * 255)
    end
end

return M
