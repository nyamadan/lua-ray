-- posteffect_worker.lua
-- ThreadWorker用のPostEffect処理スクリプト

-- _app_data, _bounds, _scene_type, _thread_id はC++から注入される

local scene_module = require("scenes." .. _scene_type)

-- ポストエフェクト処理ループ
local x_start = _bounds.x
local x_end = _bounds.x + _bounds.w - 1
local y_start = _bounds.y
local y_end = _bounds.y + _bounds.h - 1

for y = y_start, y_end do
    for x = x_start, x_end do
        scene_module.post_effect(_app_data, x, y)
    end
end
