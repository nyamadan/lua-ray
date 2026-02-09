-- worker.lua
-- ThreadWorker executes this script

-- _app_data, _scene, _bounds, _scene_type, _thread_id are injected by C++

local BlockUtils = require("lib.BlockUtils")


local scene_module = require("scenes." .. _scene_type)

-- Initialize the scene module for this worker
-- Worker threads only need to call start() to initialize camera and locals.
if scene_module.start then
    scene_module.start(_scene, _app_data)
else
    error("Scene module " .. _scene_type .. " does not have a start function")
end

-- 共有キューのキー設定
local queue_key = "render_queue"
local index_key = "render_queue_idx"

-- 動的キャンセルチェック用の変数
local time_avg = BlockUtils.MovingAverage.new(0.1) -- alpha=0.1 (指数移動平均)
local CHECK_INTERVAL_MS = 12 -- 12ms間隔でキャンセルチェック
local estimated_time = 0

while true do
    -- 次のブロックを取得
    local block = BlockUtils.pull_next_block(_app_data, queue_key, index_key)
    
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
                if _is_cancel_requested() then
                    return
                end
                estimated_time = 0
            end
            
            -- ピクセル処理開始時刻
            local start_time = app.get_ticks()
            
            scene_module.shade(_app_data, x, y)
            
            -- 処理時間を計測して移動平均を更新
            local elapsed = app.get_ticks() - start_time
            if elapsed <= 0 then
                elapsed = 0.001
            end
            time_avg:update(elapsed)
            
            -- 推定時間を更新
            estimated_time = estimated_time + time_avg:get()
        end
    end
end

-- print(string.format("Thread %d done", _thread_id))
