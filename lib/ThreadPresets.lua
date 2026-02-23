-- ThreadPresets モジュール
-- スレッド数とブロックサイズのプリセット管理

local ThreadPresets = {}

-- スレッド数プリセット
local thread_presets = {
    { value = 1, name = "1" },
    { value = 2, name = "2" },
    { value = 4, name = "4" },
    { value = 8, name = "8" },
    { value = 16, name = "16" },
    { value = 32, name = "32" },
}

-- ブロックサイズプリセット
local block_presets = {
    { value = 64, name = "64" },
    { value = 128, name = "128" },
    { value = 256, name = "256" },
    { value = 512, name = "512" },
}

-- デフォルトインデックス (8スレッド)
local default_thread_index = 4

-- デフォルトインデックス (64ブロック)
local default_block_index = 1

-- スレッド数プリセット一覧を取得
function ThreadPresets.get_thread_presets()
    return thread_presets
end

-- ブロックサイズプリセット一覧を取得
function ThreadPresets.get_block_presets()
    return block_presets
end

-- スレッド数デフォルトインデックスを取得
function ThreadPresets.get_default_thread_index()
    return default_thread_index
end

-- ブロックサイズデフォルトインデックスを取得
function ThreadPresets.get_default_block_index()
    return default_block_index
end

-- 値からスレッド数プリセットのインデックスを逆引き
function ThreadPresets.find_thread_index(value)
    for i, preset in ipairs(thread_presets) do
        if preset.value == value then
            return i
        end
    end
    return nil
end

-- 値からブロックサイズプリセットのインデックスを逆引き
function ThreadPresets.find_block_index(value)
    for i, preset in ipairs(block_presets) do
        if preset.value == value then
            return i
        end
    end
    return nil
end

return ThreadPresets
