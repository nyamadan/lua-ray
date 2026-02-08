-- BlockUtils.lua
-- ブロック分割と移動平均計算のユーティリティモジュール

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

return BlockUtils
