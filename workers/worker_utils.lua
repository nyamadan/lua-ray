-- workers/worker_utils.lua

local BlockUtils = require("lib.BlockUtils")

local WorkerUtils = {}

-- 共通のブロック処理ループ
-- @param app_data AppDataインスタンス
-- @param queue_key キューのキー名
-- @param index_key インデックスのキー名
-- @param process_callback (app_data, x, y) -> void
-- @param check_cancel_callback () -> boolean キャンセルチェック用コールバック
-- @param time_source table|nil 時間計測用オブジェクト (get_ticksメソッドを持つ)。nilの場合はglobal 'app'を使用
function WorkerUtils.process_blocks(app_data, queue_key, index_key, process_callback, check_cancel_callback, time_source)
    local timer = time_source or app
    
    -- 動的キャンセルチェック用の変数
    local time_avg = BlockUtils.MovingAverage.new(0.1) -- alpha=0.1 (指数移動平均)
    local CHECK_INTERVAL_MS = 12 -- 12ms間隔でキャンセルチェック
    local estimated_time = 0

    while true do
        -- 次のブロックを取得
        local block = BlockUtils.pull_next_block(app_data, queue_key, index_key)
        
        -- ブロックが無ければ終了
        if not block then
            break
        end

        local x_start = block.x
        local x_end = block.x + block.w - 1
        local y_start = block.y
        local y_end = block.y + block.h - 1
        
        for y = y_start, y_end do
            for x = x_start, x_end do
                -- キャンセルチェック（移動平均に基づく間隔）
                if estimated_time >= CHECK_INTERVAL_MS then
                    if check_cancel_callback() then
                        return
                    end
                    estimated_time = 0
                end
                
                -- ピクセル処理開始時刻
                local start_time = timer.get_ticks()
                
                process_callback(app_data, x, y)
                
                -- 処理時間を計測して移動平均を更新
                local elapsed = timer.get_ticks() - start_time
                if elapsed <= 0 then
                    elapsed = 0.001
                end
                time_avg:update(elapsed)
                
                -- 推定時間を更新
                estimated_time = estimated_time + time_avg:get()
            end
        end
    end
end

return WorkerUtils
