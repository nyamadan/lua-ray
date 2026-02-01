-- worker.lua
-- ThreadWorker executes this script

-- _app_data, _scene, _bounds, _scene_type, _thread_id are injected by C++

local scene_module = require("scenes." .. _scene_type)

-- Initialize the scene module for this worker
-- We pass true as the third argument to indicate this is a worker thread
-- effectively skipping geometry creation but initializing camera and locals.
if scene_module.setup then
    scene_module.setup(_scene, _app_data, true)
else
    error("Scene module " .. _scene_type .. " does not have a setup function")
end

-- Render loop
local x_start = _bounds.x
local x_end = _bounds.x + _bounds.w - 1
local y_start = _bounds.y
local y_end = _bounds.y + _bounds.h - 1

-- print(string.format("Thread %d starting: x[%d-%d] y[%d-%d]", _thread_id, x_start, x_end, y_start, y_end))

for y = y_start, y_end do
    for x = x_start, x_end do
        scene_module.shade(_app_data, x, y)
    end
end

-- print(string.format("Thread %d done", _thread_id))
