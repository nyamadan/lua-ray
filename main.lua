-- SDL3 Ray Tracing in Lua

local RayTracer = require("lib.RayTracer")

-- Main Execution Flow
local WIDTH = 800
local HEIGHT = 600

-- Instantiate
raytracer = RayTracer.new(WIDTH, HEIGHT)

-- Initialize resources
raytracer:init()

-- Initialize scene
raytracer:init_scene()

-- Initial Render is called inside init_scene -> reset_scene, but we can call it here if we want strictly explicit
-- raytracer:render() -- reset_scene already calls render

-- Set global frame callback
function app.on_frame()
    raytracer:update()
    raytracer:on_ui()
end

-- Set global quit callback (ウィンドウ終了時にワーカーを安全に停止)
function app.on_quit()
    print("Terminating workers before quit...")
    raytracer:terminate_workers()
end
