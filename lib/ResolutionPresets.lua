-- ResolutionPresets モジュール
-- 解像度プリセットとフィット計算ユーティリティ

local ResolutionPresets = {}

-- プリセット一覧
local presets = {
    { name = "320x240 (QVGA)", width = 320, height = 240 },
    { name = "640x480 (VGA)", width = 640, height = 480 },
    { name = "800x600 (SVGA)", width = 800, height = 600 },
    { name = "1280x720 (HD)", width = 1280, height = 720 },
    { name = "1920x1080 (FHD)", width = 1920, height = 1080 },
}

-- デフォルトプリセット(SVGA: 800x600)のインデックス
local default_index = 3

-- プリセット一覧を取得
function ResolutionPresets.get_presets()
    return presets
end

-- デフォルトプリセットのインデックスを取得
function ResolutionPresets.get_default_index()
    return default_index
end

-- インデックスから解像度を取得
function ResolutionPresets.get_resolution(index)
    local preset = presets[index]
    if preset then
        return preset.width, preset.height
    end
    return nil, nil
end

-- プリセット数を取得
function ResolutionPresets.count()
    return #presets
end

-- アスペクト比を維持したフィット矩形を計算
-- @param tex_w テクスチャ幅
-- @param tex_h テクスチャ高さ
-- @param win_w ウィンドウ幅
-- @param win_h ウィンドウ高さ
-- @return x, y, w, h 中央揃えされた描画矩形
function ResolutionPresets.calculate_fit_rect(tex_w, tex_h, win_w, win_h)
    local tex_aspect = tex_w / tex_h
    local win_aspect = win_w / win_h
    
    local fit_w, fit_h
    
    if tex_aspect > win_aspect then
        -- テクスチャが横長: 幅をウィンドウに合わせる
        fit_w = win_w
        fit_h = win_w / tex_aspect
    else
        -- テクスチャが縦長または同じ: 高さをウィンドウに合わせる
        fit_h = win_h
        fit_w = win_h * tex_aspect
    end
    
    -- 中央揃え
    local x = (win_w - fit_w) / 2
    local y = (win_h - fit_h) / 2
    
    return x, y, fit_w, fit_h
end

return ResolutionPresets
