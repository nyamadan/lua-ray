-- workers/ray_worker.lua
-- ThreadWorker executes this script

-- _app_data, _scene, _bounds, _scene_type, _thread_id are injected by C++

local WorkerUtils = require("workers.worker_utils")
local scene_module = require("scenes." .. _scene_type)

-- Initialize the scene module for this worker
if scene_module.start then
    scene_module.start(_scene, _app_data)
else
    error("Scene module " .. _scene_type .. " does not have a start function")
end

-- コールバック関数
local function process_callback(app_data, x, y)
    scene_module.shade(app_data, x, y)
end

-- キャンセルチェック関数
local function check_cancel()
    return _is_cancel_requested()
end

-- 処理実行
WorkerUtils.process_blocks(_app_data, "render_queue", "render_queue_idx", process_callback, check_cancel)
