local M = {}

function M.setup_or_sync_camera(camera, app_data, default_params)
    if not camera then
        local Camera = require("lib.Camera")
        camera = Camera.new("perspective", default_params)
    end
    
    -- ワーカー用にシリアライズされたカメラ状態が渡されていれば適用する
    if _thread_id ~= nil and app_data then
        local camera_state_json = app_data:get_string("camera_state")
        if camera_state_json and camera_state_json ~= "" then
            local json = require("lib.json")
            local success, state = pcall(json.decode, camera_state_json)
            if success and state then
                -- カメラのプロパティを上書き
                if state.position then camera.position = state.position end
                if state.look_at then camera.look_at = state.look_at end
                if state.up then camera.camera_up = state.up end
                if state.aspect_ratio then camera.aspect_ratio = state.aspect_ratio end
                if state.fov then camera.fov = state.fov end
                if state.type then camera.type = state.type end
                
                -- 基底ベクトルを再計算
                camera:compute_camera_basis()
            end
        end
    end
    
    return camera
end

return M
