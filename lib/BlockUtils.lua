-- BlockUtils.lua
-- ブロック分割と移動平均計算のユーティリティモジュール

local json = require("lib.json")

local BlockUtils = {}

-- ========================================
-- 指数移動平均 (Exponential Moving Average)
-- ========================================

BlockUtils.MovingAverage = {}
BlockUtils.MovingAverage.__index = BlockUtils.MovingAverage

--- 新しいMovingAverageインスタンスを作成
--- @param alpha number 平滑化係数 (0.0-1.0)。値が大きいほど新しい値への反応が速い
--- @return table MovingAverageインスタンス
function BlockUtils.MovingAverage.new(alpha)
    local self = setmetatable({}, BlockUtils.MovingAverage)
    self.alpha = alpha or 0.1
    self.value = 0.0
    self.initialized = false
    return self
end

--- 新しい値で移動平均を更新
--- @param new_value number 新しいサンプル値
function BlockUtils.MovingAverage:update(new_value)
    if not self.initialized then
        -- 初回は値をそのままセット
        self.value = new_value
        self.initialized = true
    else
        -- EMA: new_avg = alpha * new_value + (1 - alpha) * old_avg
        self.value = self.alpha * new_value + (1 - self.alpha) * self.value
    end
end

--- 現在の移動平均値を取得
--- @return number 現在の移動平均値
function BlockUtils.MovingAverage:get()
    return self.value
end

-- ========================================
-- ブロック生成
-- ========================================

--- 画面をブロックに分割し、各ブロックをスレッドに割り当てる
--- @param width number 画面幅
--- @param height number 画面高さ
--- @param block_size number ブロックサイズ（正方形）
--- @param num_threads number スレッド数
--- @return table ブロックの配列 {{x, y, w, h, thread_id}, ...}
function BlockUtils.generate_blocks(width, height, block_size, num_threads)
    local blocks = {}
    
    -- ブロック数を計算
    local blocks_x = math.ceil(width / block_size)
    local blocks_y = math.ceil(height / block_size)
    
    -- ブロックを生成
    for by = 0, blocks_y - 1 do
        for bx = 0, blocks_x - 1 do
            local x = bx * block_size
            local y = by * block_size
            local w = math.min(block_size, width - x)
            local h = math.min(block_size, height - y)
            
            table.insert(blocks, {
                x = x,
                y = y,
                w = w,
                h = h,
                thread_id = -1  -- 後で割り当て
            })
        end
    end
    
    -- ブロックをスレッドに均等に割り当て（ラウンドロビン）
    for i, block in ipairs(blocks) do
        block.thread_id = (i - 1) % num_threads
    end
    
    return blocks
end

--- スレッドIDごとにブロックをグループ化
--- @param blocks table ブロックの配列
--- @param num_threads number スレッド数
--- @return table スレッドIDをキーとしたブロック配列のテーブル
function BlockUtils.group_blocks_by_thread(blocks, num_threads)
    local groups = {}
    
    -- 初期化
    for i = 0, num_threads - 1 do
        groups[i] = {}
    end
    
    -- グループ化
    for _, block in ipairs(blocks) do
        local tid = block.thread_id
        table.insert(groups[tid], block)
    end
    
    return groups
end

-- ========================================
-- シャッフル
-- ========================================

--- ブロック配列をシャッフルする (Fisher-Yates シャッフル)
--- @param blocks table ブロックの配列
--- @param seed number|nil 乱数シード (nil の場合は現在時刻をシードとして使用)
--- @return table シャッフルされたブロック配列（新規テーブル）
function BlockUtils.shuffle_blocks(blocks, seed)
    -- シードを設定
    if seed then
        math.randomseed(seed)
    else
        math.randomseed(os.time())
    end
    
    -- 元の配列をコピー
    local shuffled = {}
    for i, block in ipairs(blocks) do
        shuffled[i] = block
    end
    
    -- Fisher-Yates シャッフル
    local n = #shuffled
    for i = n, 2, -1 do
        local j = math.random(1, i)
        shuffled[i], shuffled[j] = shuffled[j], shuffled[i]
    end
    
    return shuffled
end

-- ========================================
-- ソート
-- ========================================

--- ブロック配列を指定された座標に近いものから順にソートする
--- 元の配列は変更せず、新しい配列を返す
--- @param blocks table ブロックの配列
--- @param center_x number 基準となるX座標
--- @param center_y number 基準となるY座標
--- @return table ソートされたブロック配列（新規テーブル）
function BlockUtils.sort_blocks_by_distance(blocks, center_x, center_y)
    -- 元の配列をコピー
    local sorted = {}
    for i, block in ipairs(blocks) do
        -- ブロックの中心座標を計算して距離を事前計算
        local bx = block.x + block.w / 2
        local by = block.y + block.h / 2
        local dist_sq = (bx - center_x) * (bx - center_x) + (by - center_y) * (by - center_y)
        
        table.insert(sorted, {
            block = block,
            dist_sq = dist_sq,
            original_index = i -- 安定ソートのため
        })
    end
    
    -- 距離の二乗でソート
    table.sort(sorted, function(a, b)
        if a.dist_sq == b.dist_sq then
            return a.original_index < b.original_index
        end
        return a.dist_sq < b.dist_sq
    end)
    
    -- 元のブロック形式に戻す
    local result = {}
    for i, item in ipairs(sorted) do
        result[i] = item.block
    end
    
    return result
end

-- ========================================
-- 共有キュー（排他的pull方式）
-- ========================================

--- 共有ブロックキューをセットアップする
--- ブロック配列をJSONでAppDataに保存し、インデックスを0に初期化
--- @param app_data userdata AppDataインスタンス
--- @param blocks table ブロックの配列
--- @param queue_key string キュー（ブロック配列）のキー
--- @param index_key string|nil インデックスのキー（省略時はqueue_key.."_idx"）
function BlockUtils.setup_shared_queue(app_data, blocks, queue_key, index_key)
    index_key = index_key or (queue_key .. "_idx")
    
    -- ブロック配列をJSONで保存
    app_data:set_string(queue_key, json.encode(blocks))
    -- インデックスを0にリセット
    app_data:set_string(index_key, "0")
end

--- 共有キューから次のブロックを排他的に取得する
--- @param app_data userdata AppDataインスタンス
--- @param queue_key string キュー（ブロック配列）のキー
--- @param index_key string|nil インデックスのキー（省略時はqueue_key.."_idx"）
--- @return table|nil ブロック情報、無ければnil
function BlockUtils.pull_next_block(app_data, queue_key, index_key)
    index_key = index_key or (queue_key .. "_idx")
    
    -- インデックスを排他的に取得してインクリメント
    local index = app_data:pop_next_index(index_key)
    
    -- ブロック配列を取得（キャッシュを考慮するなら改善の余地あり）
    local queue_json = app_data:get_string(queue_key)
    if not queue_json or queue_json == "" then
        return nil
    end
    
    local blocks = json.decode(queue_json)
    
    -- Luaは1-indexedなので+1
    local block = blocks[index + 1]
    return block
end

return BlockUtils
