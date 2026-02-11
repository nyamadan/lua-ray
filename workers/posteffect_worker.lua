-- workers/posteffect_worker.lua
-- ThreadWorker executes this script

-- _app_data, _bounds, _scene_type, _thread_id are injected by C++

local WorkerUtils = require("workers.worker_utils")
local scene_module = require("scenes." .. _scene_type)

-- コールバック関数
local function process_callback(app_data, x, y)
    scene_module.post_effect(app_data, x, y)
end

-- キャンセルチェック関数
local function check_cancel()
    return _is_cancel_requested()
end

-- 処理実行
WorkerUtils.process_blocks(_app_data, "posteffect_queue", "posteffect_queue_idx", process_callback, check_cancel)
